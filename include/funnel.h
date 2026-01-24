#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @file */

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

/** Helper to create a funnel_fraction
 @param num Numerator
 @param den Denominator
 @returns The funnel_fraction

 */
static inline struct funnel_fraction FUNNEL_FRACTION(uint32_t num,
                                                     uint32_t den) {
    return (struct funnel_fraction){num, den};
}

/** A user callback for buffer creation/destruction
 *
 * @param opaque Opaque user data pointer
 * @param stream Stream for this buffer @borrowed
 * @param buf Buffer being allocated or freed @borrowed
 */
typedef void (*funnel_buffer_callback)(void *opaque,
                                       struct funnel_stream *stream,
                                       struct funnel_buffer *buf);

/**
 * Synchronization modes for the frame pacing
 */
enum funnel_mode {
    /**
     * Produce frames asynchronously to the consumer.
     *
     * In this mode, libfunnel calls never block and you
     * must be able to handle the lack of a buffer (by
     * skipping rendering/copying to it). This mode only
     * makes sense if your application is FPS-limited by
     * some other consumer (for example, if it renders to
     * the screen, usually with VSync). You should configure
     * the frame rate you expect to produce frames at with
     * `funnel_stream_set_rate()`.
     *
     * This mode essentially behaves like triple buffering.
     * Whenever the PipeWire cycle runs, the consumer will
     * receive the frame that was most recently submitted
     * to funnel_stream_enqueue().
     */
    FUNNEL_ASYNC,
    /**
     * Produce frames synchronously to the consumer with
     * double buffering.
     *
     * In this mode, after a frame is produced, it is
     * queued to be sent out to the consumer in the next
     * PipeWire process cycle, and you may immediately
     * dequeue a new buffer to start rendering the next
     * frame. libfunnel will block at `funnel_stream_enqueue()`
     * until the previously queued frame has been consumed.
     * In this mode, `funnel_stream_dequeue()` will only
     * block if there are no free buffers (if the consumer is
     * not freeing buffers quickly enough).
     *
     * This mode effectively adds two frames of latency,
     * as up to two frames can be rendered ahead of the
     * PipeWire cycle (one ready to be submitted, and
     * one blocked at `funnel_stream_enqueue()`).
     */
    FUNNEL_DOUBLE_BUFFERED,
    /**
     * Produce frames synchronously to the consumer with
     * single buffering.
     *
     * In this mode, after a frame is produced, it is
     * queued to be sent out to the consumer in the next
     * PipeWire process cycle. When you are ready to begin
     * rendering a new frame, libfunnel will block
     * at `funnel_stream_dequeue()` until the previous frame
     * has been sent to the consumer. In this mode,
     * `funnel_stream_enqueue()` will never block.
     *
     * This mode effectively adds one frame of latency,
     * as only one frame can be rendered ahead of the
     * PipeWire cycle.
     */
    FUNNEL_SINGLE_BUFFERED,
    /**
     * Produce frames synchronously with the PipeWire process
     * cycle.
     *
     * In this mode, `funnel_stream_dequeue()` will wait for
     * the beginning of a PipeWire process cycle, and the
     * process cycle will be blocked until the frame is
     * submitted with `funnel_stream_enqueue()`.
     *
     * This mode provides the lowest possible latency, but
     * is only suitable for applications that do not do much
     * work to render frames (for example, just a copy), as
     * the PipeWire graph will be blocked while the buffer
     * is dequeued. It adds no latency.
     */
    FUNNEL_SYNCHRONOUS,
};

/**
 * Buffer synchronization modes for frames
 */
enum funnel_sync {
    /**
     * Use implicit buffer sync only.
     *
     * This will only advertise implicit sync on
     * the PipeWire stream. The other end must
     * support implicit sync.
     *
     * Explicit sync APIs are not available on
     * buffers.
     *
     * Not available for Vulkan. Does not work on
     * the NVidia proprietary driver.
     */
    FUNNEL_SYNC_IMPLICIT,

    /**
     * Use explicit buffer sync, with automatic
     * conversion to implicit sync.
     *
     * Advertise both implicit and explicit
     * sync, and negotiate automatically depending
     * on the capabilities of the other end.
     *
     * You must use explicit sync APIs to
     * synchronize buffer access.
     */
    FUNNEL_SYNC_EXPLICIT_HYBRID,
    /**
     * Use explicit buffer sync only.
     *
     * This will only advertise explicit sync on
     * the PipeWire stream. The other end must
     * support explicit sync, or else stream
     * negotiation will fail.
     *
     * You must use explicit sync APIs to
     * synchronize buffer access.
     */
    FUNNEL_SYNC_EXPLICIT_ONLY,
    /**
     * Support both explicit and implicit sync.
     *
     * Advertise both implicit and explicit
     * sync, and negotiate automatically depending
     * on the capabilities of the other end.
     *
     * You must query the sync type for each
     * dequeued funnel_buffer, and use explicit
     * sync APIs if the buffer has explicit sync
     * enabled.
     *
     * Not available for Vulkan.
     */
    FUNNEL_SYNC_EITHER,
};

/**
 * Create a Funnel context.
 *
 * As multiple Funnel contexts are completely independent, this function has no synchronization requirements.
 *
 * @param[out] pctx New context @owned
 * @return_err
 * @retval -ECONNREFUSED Failed to connect to PipeWire daemon
 */
int funnel_init(struct funnel_ctx **pctx);

/**
 * Shut down a Funnel context.
 *
 * @sync-ext
 *
 * @param ctx Context @owned
 */
void funnel_shutdown(struct funnel_ctx *ctx);

/**
 * Create a new stream.
 *
 * @sync-int
 *
 * @param ctx Context @borrowed
 * @param name Name of the new stream @borrowed
 * @param[out] pstream New stream @owned-from{ctx}
 * @return_err
 * @retval -EIO The PipeWire context is invalid (fatal error)
 */
int funnel_stream_create(struct funnel_ctx *ctx, const char *name,
                         struct funnel_stream **pstream);

/**
 * Specify callbacks for buffer creation/destruction.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @param alloc Callback when a buffer is allocated @borrowed-by{stream}
 * @param free Callback when a buffer is freed @borrowed-by{stream}
 * @param opaque Opaque user pointer
 */
void funnel_stream_set_buffer_callbacks(struct funnel_stream *stream,
                                        funnel_buffer_callback alloc,
                                        funnel_buffer_callback free,
                                        void *opaque);

/**
 * Set the frame dimensions for a stream.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @param width Width in pixels
 * @param height Height in pixels
 * @return_err
 * @retval -EINVAL Invalid argument
 */
int funnel_stream_set_size(struct funnel_stream *stream, uint32_t width,
                           uint32_t height);

/**
 * Configure the queueing mode for the stream.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @param mode Queueing mode for the stream
 * @return_err
 * @retval -EINVAL Invalid argument
 */
int funnel_stream_set_mode(struct funnel_stream *stream, enum funnel_mode mode);

/**
 * Configure the synchronization mode for the stream.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @param sync Synchronization mode for the stream
 * @return_err
 * @retval -EOPNOTSUPP The selected API does not support this sync mode
 */
int funnel_stream_set_sync(struct funnel_stream *stream, enum funnel_sync sync);

/**
 * Set the frame rate of a stream.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @param def Default frame rate (FUNNEL_RATE_VARIABLE for no default or
 * variable)
 * @param min Minimum frame rate (FUNNEL_RATE_VARIABLE if variable)
 * @param max Maximum frame rate (FUNNEL_RATE_VARIABLE if variable)
 * @return_err
 * @retval -EINVAL Invalid argument
 */
int funnel_stream_set_rate(struct funnel_stream *stream,
                           struct funnel_fraction def,
                           struct funnel_fraction min,
                           struct funnel_fraction max);

/**
 * Get the currently negotiated frame rate of a stream.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @param[out] prate Output frame rate
 * @return_err
 * @retval -EINPROGRESS The stream is not yet initialized
 */
int funnel_stream_get_rate(struct funnel_stream *stream,
                           struct funnel_fraction *prate);

/**
 * Clear the supported format list. Used for reconfiguration.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 */
void funnel_stream_clear_formats(struct funnel_stream *stream);

/**
 * Apply the stream configuration and register the stream with PipeWire.
 *
 * If called on an already configured stream, this will update the
 * configuration.
 *
 * @sync-ext
 *
 * @param stream Stream @borrowed
 * @return_err
 * @retval -EINVAL The stream is in an invalid state (missing settings)
 * @retval -EIO The PipeWire context is invalid or stream creation failed
 */
int funnel_stream_configure(struct funnel_stream *stream);

/**
 * Start running a stream.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @return_err
 * @retval -EINVAL The stream is in an invalid state (not configured)
 * @retval -EIO The PipeWire context is invalid or stream creation failed
 */
int funnel_stream_start(struct funnel_stream *stream);

/**
 * Stop running a stream.
 *
 * If another thread is blocked on funnel_stream_dequeue(), this will
 * unblock it.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @return_err
 * @retval -EINVAL The stream is not started
 * @retval -EIO The PipeWire context is invalid
 */
int funnel_stream_stop(struct funnel_stream *stream);

/**
 * Destroy a stream.
 *
 * The stream will be stopped if it is running.
 *
 * @sync-ext
 *
 * @param stream Stream @owned
 */
void funnel_stream_destroy(struct funnel_stream *stream);

/**
 * Dequeue a buffer from a stream.
 *
 * Note that, currently, you may only have one buffer
 * dequeued at a time.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @param[out] pbuf Buffer that was dequeued @owned-from{stream}
 * @return Whether a buffer was dequeued successfully, or a negative error
 * number on error.
 * @retval 0 No buffer is available
 * @retval 1 A buffer was successfully dequeued
 * @retval -EINVAL Stream is in an invalid state
 * @retval -EBUSY Attempted to dequeue more than one buffer at once
 * @retval -EIO The PipeWire context is invalid
 * @retval -ESHUTDOWN Stream is not started
 */
int funnel_stream_dequeue(struct funnel_stream *stream,
                          struct funnel_buffer **pbuf);

/**
 * Enqueue a buffer to a stream.
 *
 * After this call, the buffer is no longer owned by the user and may not be
 * queued again until it is dequeued.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @param buf Buffer to enqueue @owned
 * @return Whether a buffer was enqueued successfully, or a negative error
 * number on error.
 * @retval 0 The buffer was dropped because the stream configuration or state
 * changed.
 * @retval 1 The buffer was successfully enqueued.
 * @retval -EINVAL
 *  * Invalid argument
 *  * Stream is in an invalid state (not yet configured)
 *  * Buffer requires sync, but sync was not handled properly
 * @retval -EIO The PipeWire context is invalid
 * @retval -ESHUTDOWN Stream is not started
 */
int funnel_stream_enqueue(struct funnel_stream *stream,
                          struct funnel_buffer *buf);

/**
 * Return a buffer to the pool without enqueueing it.
 *
 * After this call, the buffer is no longer owned by the user and may not be
 * queued again until it is dequeued. This will effectively drop one frame.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @param buf Buffer to return @owned
 * @return_err
 * @retval -EINVAL
 *  * Invalid argument
 *  * Stream is in an invalid state (not yet configured)
 * @retval -EIO The PipeWire context is invalid
 * @retval -ESHUTDOWN Stream is not started
 */
int funnel_stream_return(struct funnel_stream *stream,
                         struct funnel_buffer *buf);

/**
 * Skip a frame for a stream
 *
 * This call forces at least one subsequent call to funnel_stream_dequeue()
 * to return without a buffer. This is useful to break a thread out of
 * that function.
 *
 * @sync-int
 *
 * @param stream Stream @borrowed
 * @return_err
 * @retval -EINVAL Stream is in an invalid state (not yet configured)
 */
int funnel_stream_skip_frame(struct funnel_stream *stream);

/**
 * Get the dimensions of a Funnel buffer.
 *
 * @sync-ext
 *
 * @param buf Buffer @borrowed
 * @param[out] pwidth Output width
 * @param[out] pheight Output height
 */
void funnel_buffer_get_size(struct funnel_buffer *buf, uint32_t *pwidth,
                            uint32_t *pheight);

/**
 * Set an arbitrary user data pointer for a buffer.
 *
 * The user is responsible for managing the lifetime of this object.
 * Generally, you should use funnel_stream_set_buffer_callbacks()
 * to provide buffer creation/destruction callbacks, and set and
 * release the user data pointer in the alloc and free callback
 * respectively.
 *
 * @sync-ext
 *
 * @param buf Buffer @borrowed
 * @param opaque Opaque user data pointer
 */
void funnel_buffer_set_user_data(struct funnel_buffer *buf, void *opaque);

/**
 * Get an arbitrary user data pointer for a buffer.
 *
 * @sync-ext
 *
 * @param buf Buffer @borrowed
 * @return The user data pointer
 */
void *funnel_buffer_get_user_data(struct funnel_buffer *buf);

/**
 * Check whether a buffer requires explicit synchronization.
 *
 * @sync-ext
 *
 * @param buf Buffer @borrowed
 * @retval true if the buffer requires explicit synchronization
 */
bool funnel_buffer_has_sync(struct funnel_buffer *buf);

/**
 * Return whether a buffer is considered efficient for rendering.
 *
 * Buffers are considered efficient when they are not using linear tiling
 * and non-linear tiling is supported by the GPU driver.
 *
 * @sync-ext
 *
 * @param buf Buffer @borrowed
 * @retval true if the buffer is likely to be efficient to render into
 * @retval false if the buffer is unlikely to be efficient to render into
 */
bool funnel_buffer_is_efficient_for_rendering(struct funnel_buffer *buf);
