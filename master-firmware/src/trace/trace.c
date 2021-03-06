#include "trace.h"
#include <stddef.h>
#include <string.h>

#ifdef TESTS
#define TRACE_LOCK()
#define TRACE_UNLOCK()
unsigned int timestamp = 0;
#define TRACE_TIMESTAMP_GET() timestamp
#else /* ChibiOS */
#include <ch.h>
#define TRACE_LOCK() syssts_t __syssst = chSysGetStatusAndLockX()
#define TRACE_UNLOCK() chSysRestoreStatusX(__syssst)
#define TRACE_TIMESTAMP_GET() ST2MS(chVTGetSystemTimeX())
#endif

struct trace_buffer_struct trace_buffer;

static void trace_push_event(uint8_t event_id, struct trace_event *event)
{
    if (!trace_buffer.active) {
        return;
    }
    event->event_id = event_id;
    event->timestamp = TRACE_TIMESTAMP_GET();

    TRACE_LOCK();
    struct trace_event *p;
    p = &trace_buffer.data[trace_buffer.write_index];
    trace_buffer.write_index = (trace_buffer.write_index + 1) % TRACE_BUFFER_SIZE;
    if (trace_buffer.nb_events < TRACE_BUFFER_SIZE) {
        trace_buffer.nb_events += 1;
    }
    *p = *event;
    TRACE_UNLOCK();
}

void trace(uint8_t event_id)
{
    struct trace_event e;
    e.type = TRACE_TYPE_STRING;
    e.data.string = "";
    trace_push_event(event_id, &e);
}

void trace_address(uint8_t event_id, void *p)
{
    struct trace_event e;
    e.type = TRACE_TYPE_ADDRESS;
    e.data.address = p;
    trace_push_event(event_id, &e);
}

void trace_string(uint8_t event_id, const char *str)
{
    struct trace_event e;
    e.type = TRACE_TYPE_STRING;
    e.data.string = str;
    trace_push_event(event_id, &e);
}

void trace_scalar(uint8_t event_id, float f)
{
    struct trace_event e;
    e.type = TRACE_TYPE_SCALAR;
    e.data.scalar = f;
    trace_push_event(event_id, &e);
}

void trace_integer(uint8_t event_id, int32_t i)
{
    struct trace_event e;
    e.type = TRACE_TYPE_INTEGER;
    e.data.integer = i;
    trace_push_event(event_id, &e);
}

/** Initialize the trace system
 *
 * @note Tracing should be initialized
 */
void trace_init(void)
{
    memset(&trace_buffer, 0, sizeof(trace_buffer));
}

/** Enable trace */
void trace_enable(void)
{
    trace_buffer.active = true;
}

/** Disable trace
 *
 * @note Trace calls are still allowed, they just are not saved.
 */
void trace_disable(void)
{
    trace_buffer.active = false;
}

/** Clear the trace buffer */
void trace_clear(void)
{
    TRACE_LOCK();
    trace_buffer.write_index = 0;
    trace_buffer.nb_events = 0;
    TRACE_UNLOCK();
}

void trace_print(void (*print_fn)(void *, const char *, ...), void *arg)
{
    trace_buffer.active = false;
    size_t i;
    size_t index;
    if (trace_buffer.nb_events == TRACE_BUFFER_SIZE) {
        index = trace_buffer.write_index;
    } else {
        index = 0;
    }
    for (i = 0; i < trace_buffer.nb_events; i++) {
        struct trace_event *e = &trace_buffer.data[index];
        print_fn(arg, "[%u] %s: ", e->timestamp, event_names[e->event_id]);
        switch (e->type) {
        case TRACE_TYPE_STRING:
            print_fn(arg, "\"%s\"\n", e->data.string);
            break;
        case TRACE_TYPE_ADDRESS:
#ifdef _CHIBIOS_RT_ // workaround for chprintf
            print_fn(arg, "%x\n", e->data.address);
#else
            print_fn(arg, "%p\n", e->data.address);
#endif
            break;
        case TRACE_TYPE_SCALAR:
            print_fn(arg, "%f\n", e->data.scalar);
            break;
        case TRACE_TYPE_INTEGER:
            print_fn(arg, "%d\n", e->data.integer);
            break;
        };
        index = (index + 1) % TRACE_BUFFER_SIZE;
    }
    trace_buffer.active = true;
}
