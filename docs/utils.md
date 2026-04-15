# utils

Minimal utility helpers used across all libraries.

**Header:** `lib/utils/utils.h`

---

## API

### `int MAX(int a, int b)`

Returns the larger of two integers.

```c
int brightness = MAX(0, raw - offset);
```

### `int MIN(int a, int b)`

Returns the smaller of two integers.

```c
int clamped = MIN(value, 255);
```

### `void delay(int ms)`

Blocks the calling FreeRTOS task for `ms` milliseconds.
Wraps `vTaskDelay(pdMS_TO_TICKS(ms))`.

```c
delay(500); // wait 500 ms
```

---

## Notes

- `MAX` and `MIN` are plain functions, not macros — they evaluate each argument exactly once.
- `delay(0)` yields to the scheduler (equivalent to `taskYIELD()`).
