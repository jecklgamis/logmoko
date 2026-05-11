## Concurrency Patterns in Logmoko's Producer/Consumer Design

Each handler runs a background consumer thread that drains a ring buffer and
performs all I/O. The caller (producer) enqueues log records and returns
immediately. Several well-known concurrency patterns are composed together to
make this safe and efficient.

---

### 1. Ring Buffer (Circular Buffer)

Fixed-size array with `head` and `tail` indices. The producer advances `head`,
the consumer advances `tail`. Slots are read in place using atomics — no extra
copying. When `head` catches up to `tail` the buffer is full; when `tail`
catches up to `head` it is empty.

---

### 2. Condition Variable with Spurious Wakeup Guard

The standard `while (condition) pthread_cond_wait(...)` pattern. The `while`
instead of `if` guards against spurious wakeups that POSIX explicitly permits.
Without it a false wakeup could cause the consumer to run with an empty ring.

```c
while (!lmk_ring_peek_slot(flh->ring_buffer, flh->tail, flh->ring_mask) &&
       atomic_load_explicit(&flh->running, memory_order_relaxed))
    pthread_cond_wait(&flh->wakeup_cond, &flh->wakeup_lock);
```

---

### 3. Sleeping Flag + Sequential Consistency (Double-Check Handshake)

The `sleeping` atomic with `memory_order_seq_cst` is a **flag-before-sleep**
pattern that prevents missed wakeups:

- Consumer stores `sleeping = 1` (seq_cst) before parking.
- Producer checks `sleeping` before deciding whether to signal.

The seq_cst fence makes both sides agree on ordering. Without it there is a
window where the producer reads `sleeping = 0`, skips the signal, and the
consumer parks forever with data sitting in the ring.

```c
atomic_store_explicit(&flh->sleeping, 1, memory_order_seq_cst);
// ... cond_wait ...
atomic_store_explicit(&flh->sleeping, 0, memory_order_relaxed);
```

---

### 4. Mutex-Protected Signal (Wakeup Lock)

The producer holds `wakeup_lock` when calling `cond_signal`, and the consumer
holds it when storing `sleeping = 1` and calling `cond_wait`. This closes the
TOCTOU window between "check sleeping flag" and "signal" on the producer side.
`cond_wait` atomically releases the mutex and parks the thread, so no signal
can be lost between the check and the sleep.

---

### 5. Drop-on-Full (Loss-Tolerant Producer)

When the ring is full the producer increments `dropped` and returns
immediately — it never blocks or spins. This is the **bounded drop** pattern
used in high-throughput logging, metrics pipelines, and network packet
processing where losing data under overload is preferable to back-pressuring
the caller.

```c
if (!lmk_ring_enqueue(...)) {
    atomic_fetch_add_explicit(&handler->dropped, 1, memory_order_relaxed);
    return;
}
```

---

### 6. Batch Drain with Coalesced I/O

The consumer drains *all* available slots in one pass before sleeping again.
Log lines are accumulated into a staging write buffer (`wbuf`) and flushed to
disk in a single `fwrite`/`fflush` syscall at the end of the batch. This
amortises syscall and lock overhead across many log records and is the primary
driver of write throughput.

```c
while ((slot = lmk_ring_peek_slot(...))) {
    /* format into wbuf, flush wbuf to disk only when nearly full */
    lmk_ring_consume(...);
}
/* final flush of remaining staged bytes */
fwrite(wbuf, 1, wpos, flh->log_fp);
fflush(flh->log_fp);
```

---

### 7. Lock Splitting

Two separate locks serve different purposes:

| Lock | Protects | Held by |
|---|---|---|
| `handler->lock` | Ring buffer enqueue (`head`, `count`) | Producer during enqueue |
| `wakeup_lock` | Sleep/wake signalling (`sleeping`, `cond_wait`) | Both sides during sleep/wake |

Splitting them means the producer's enqueue path does not contend with the
consumer's sleep/wake protocol. The short critical section on `handler->lock`
keeps producer latency bounded regardless of whether the consumer is sleeping.

---

### How They Compose

```
Producer                          Consumer
────────                          ────────
lock(handler->lock)               drain ring (lockless peek/consume)
  enqueue to ring                   format lines into wbuf
  inc nr_log_calls                  flush wbuf when full
unlock(handler->lock)             if ring empty:
                                    lock(wakeup_lock)
check sleeping flag ◄───────────    sleeping = 1  (seq_cst fence)
  if sleeping:                      while ring empty && running:
    lock(wakeup_lock)                 cond_wait
    cond_signal        ────────►    sleeping = 0
    unlock(wakeup_lock)             unlock(wakeup_lock)
                                  fwrite/fflush remaining wbuf
```

The `seq_cst` fence on `sleeping = 1` is the linchpin — it guarantees the
producer's subsequent read of `sleeping` sees the correct value, so the signal
is never missed.
