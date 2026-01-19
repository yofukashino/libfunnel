#pragma once

#include "funnel.h"
#include <gbm.h>

/**
 * Set up a stream for GBM integration.
 *
 * @param stream Stream
 * @param gbm_fd File descriptor of the GBM device (borrow, must be closed by
 * caller at any time)
 */
int funnel_stream_init_gbm(struct funnel_stream *stream, int gbm_fd);

/**
 * Add a supported GBM format. Must be called in preference order (highest to
 * lowest).
 *
 * @param stream Stream
 * @param format DRM format (FOURCC)
 * @param modifiers Pointer to a list of modifiers (borrow)
 * @param num_modifiers Number of modifiers passed
 */
int funnel_stream_gbm_add_format(struct funnel_stream *stream,
                                    uint32_t format, uint64_t *modifiers,
                                    size_t num_modifiers);

/**
 * Get the GBM buffer object for a Funnel buffer.
 *
 * The BO will only be valid until `buf` is returned or enqueued, or the
 * stream is destroyed.
 *
 * @param buf Buffer
 * @param bo Output GBM BO for the buffer (borrowed)
 */
int funnel_buffer_get_gbm_bo(struct funnel_buffer *buf, struct gbm_bo **bo);
