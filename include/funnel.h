#pragma once

#include <stddef.h>
#include <stdint.h>

struct funnel_ctx;
struct funnel_stream;
struct funnel_buffer;

/**
 * A rational frame rate
 */
struct funnel_fraction {
    uint32_t num;
    uint32_t den;
};

/**
 * Indicates that the frame rate is variable
 */
static const struct funnel_fraction FUNNEL_RATE_VARIABLE = {0, 1};

static inline struct funnel_fraction FUNNEL_FRACTION(uint32_t num,
                                                     uint32_t den) {
    return (struct funnel_fraction){num, den};
}

/**
 * Create a Funnel context.
 *
 * @param ctx New context
 */
int funnel_init(struct funnel_ctx **pctx);

/**
 * Shut down a Funnel context.
 *
 * @param ctx Context
 */
void funnel_shutdown(struct funnel_ctx *ctx);

/**
 * Create a new stream.
 *
 * @param ctx Context
 * @param name Name of the new stream (borrow)
 * @param stream New stream (must not outlive context)
 */
int funnel_stream_create(struct funnel_ctx *ctx, const char *name,
                         struct funnel_stream **pstream);

/**
 * Set the buffer count for a stream.
 *
 * @param stream Stream
 * @param def Default number of buffers (must be between `min` and `max)
 * @param min Minimum number of buffers (must be at least 1)
 * @param max Maximum number of buffers (must be greated than or equal to `min`)
 */
int funnel_stream_set_buffers(struct funnel_stream *stream, int def, int min,
                              int max);

/**
 * Set the frame dimensions for a stream.
 *
 * @param stream Stream
 * @param width Width in pixels
 * @param height Height in pixels
 */
int funnel_stream_set_size(struct funnel_stream *stream, uint32_t width,
                           uint32_t height);

/**
 * Set the frame rate of a stream.
 *
 * @param stream Stream
 * @param def Default frame rate (FUNNEL_RATE_VARIABLE for no default or
 * variable)
 * @param min Minimum frame rate (FUNNEL_RATE_VARIABLE if variable)
 * @param max Maximum frame rate (FUNNEL_RATE_VARIABLE if variable)
 */
int funnel_stream_set_rate(struct funnel_stream *stream,
                           struct funnel_fraction def,
                           struct funnel_fraction min,
                           struct funnel_fraction max);

/**
 * Get the currently negotiated frame rate of a stream.
 *
 * @param stream Stream
 * @param rate Output frame rate
 */
int funnel_stream_get_rate(struct funnel_stream *stream,
                           struct funnel_fraction *prate);

/**
 * Clear the supported format list. Used for reconfiguration.
 *
 * @param stream Stream
 */
void funnel_stream_clear_formats(struct funnel_stream *stream);

/**
 * Apply the stream configuration and register the stream with PipeWire.
 *
 * If called on an already configured stream, this will update the
 * configuration.
 *
 * @param stream Stream
 */
int funnel_stream_configure(struct funnel_stream *stream);

/**
 * Start running a stream.
 *
 * @param stream Stream
 */
int funnel_stream_start(struct funnel_stream *stream);

/**
 * Stop running a stream.
 *
 * @param stream Stream
 */
int funnel_stream_stop(struct funnel_stream *stream);

/**
 * Destroy a stream.
 *
 * The stream will be stopped if it is running.
 *
 * @param stream Stream
 */
void funnel_stream_destroy(struct funnel_stream *stream);

/**
 * Dequeue a buffer from a stream.
 *
 * @param stream Stream
 * @param buf Output buffer (NULL if no buffer is available)
 */
int funnel_stream_dequeue(struct funnel_stream *stream,
                          struct funnel_buffer **pbuf);

/**
 * Enqueue a buffer to a stream.
 *
 * After this call, the buffer is no longer owned by the user and may not be
 * queued again until it is dequeued.
 *
 * @param stream Stream
 * @param buf Buffer to enqueue (must have been dequeued)
 */
int funnel_stream_enqueue(struct funnel_stream *stream,
                          struct funnel_buffer *buf);

/**
 * Return a buffer to the pool without enqueueing it.
 *
 * After this call, the buffer is no longer owned by the user and may not be
 * queued again until it is dequeued.
 *
 * @param stream Stream
 * @param buf Buffer to return (must have been dequeued)
 */
int funnel_stream_return(struct funnel_stream *stream,
                         struct funnel_buffer *buf);

/**
 * Get the dimensions of a Funnel buffer.
 *
 * @param buf Buffer
 * @param width Output width
 * @param height Output height
 */
void funnel_buffer_get_size(struct funnel_buffer *buf, uint32_t *pwidth,
                            uint32_t *pheight);
