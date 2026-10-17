/**
 * @file spa_eax_node.cpp
 * @brief EXPERIMENTAL SCAFFOLD: SPA (Simple Plugin Architecture) module plugin.
 *
 * NOTE: This SPA node module (libspa-eax-filter.so) is an experimental architectural
 * scaffold for factory enumeration (audio.filter.eax) and interface validation.
 * Full in-graph port negotiation (SPA_PARAM_PortConfig) and buffer exchange are
 * scheduled for completion in Phase 4.
 *
 * For real-time audio routing and gaming integration in Phase 3, use the primary
 * eax_pipewire_filter_s API (eax_pipewire_filter.cpp) which integrates via pw_filter.
 */

#include <spa/support/plugin.h>
#include <spa/node/node.h>
#include <spa/node/io.h>
#include <spa/param/param.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/dsp-utils.h>
#include <spa/utils/defs.h>
#include <spa/utils/dict.h>
#include <spa/utils/hook.h>
#include "eax_native_abi.h"
#include <cstring>
#include <new>
#include <atomic>

#define EAX_SPA_FACTORY_NAME "audio.filter.eax"
#define EAX_SPA_MAX_PORTS    2

struct spa_eax_node {
    struct spa_node node;
    struct spa_hook_list hooks;
    struct spa_io_buffers* io_in;
    struct spa_io_buffers* io_out;
    eax_context_h eax_ctx;
    uint32_t sample_rate;
    float rt_scratch[4096 * 2];
};

struct spa_eax_handle {
    struct spa_handle handle;
    struct spa_eax_node node;
};

static int eax_node_add_listener(void* object,
                                 struct spa_hook* listener,
                                 const struct spa_node_events* events,
                                 void* data) {
    auto* self = static_cast<spa_eax_node*>(object);
    if (!self) return -EINVAL;
    spa_hook_list_append(&self->hooks, listener, events, data);
    return 0;
}

static int eax_node_set_callbacks(void* object,
                                  const struct spa_node_callbacks* callbacks,
                                  void* data) {
    (void)object;
    (void)callbacks;
    (void)data;
    return 0;
}

static int eax_node_sync(void* object, int seq) {
    (void)object;
    return seq;
}

static int eax_node_enum_params(void* object, int seq,
                                uint32_t id, uint32_t start, uint32_t max,
                                const struct spa_pod* filter) {
    (void)object;
    (void)seq;
    (void)id;
    (void)start;
    (void)max;
    (void)filter;
    return 0;
}

static int eax_node_set_param(void* object, uint32_t id, uint32_t flags, const struct spa_pod* param) {
    (void)object;
    (void)id;
    (void)flags;
    (void)param;
    return 0;
}

static int eax_node_set_io(void* object, uint32_t id, void* data, size_t size) {
    auto* self = static_cast<spa_eax_node*>(object);
    if (!self) return -EINVAL;
    (void)id;
    (void)data;
    (void)size;
    return 0;
}

static int eax_node_send_command(void* object, const struct spa_command* command) {
    (void)object;
    (void)command;
    return 0;
}

static int eax_node_add_port(void* object, enum spa_direction direction, uint32_t port_id, const struct spa_dict* props) {
    (void)object;
    (void)direction;
    (void)port_id;
    (void)props;
    return -ENOTSUP;
}

static int eax_node_remove_port(void* object, enum spa_direction direction, uint32_t port_id) {
    (void)object;
    (void)direction;
    (void)port_id;
    return -ENOTSUP;
}

static int eax_node_port_enum_params(void* object, int seq,
                                     enum spa_direction direction, uint32_t port_id,
                                     uint32_t id, uint32_t start, uint32_t max,
                                     const struct spa_pod* filter) {
    (void)object;
    (void)seq;
    (void)direction;
    (void)port_id;
    (void)id;
    (void)start;
    (void)max;
    (void)filter;
    return 0;
}

static int eax_node_port_set_param(void* object,
                                   enum spa_direction direction, uint32_t port_id,
                                   uint32_t id, uint32_t flags,
                                   const struct spa_pod* param) {
    (void)object;
    (void)direction;
    (void)port_id;
    (void)id;
    (void)flags;
    (void)param;
    return 0;
}

static int eax_node_port_use_buffers(void* object,
                                     enum spa_direction direction, uint32_t port_id,
                                     uint32_t flags,
                                     struct spa_buffer** buffers, uint32_t n_buffers) {
    (void)object;
    (void)direction;
    (void)port_id;
    (void)flags;
    (void)buffers;
    (void)n_buffers;
    return 0;
}

static int eax_node_port_set_io(void* object,
                                enum spa_direction direction, uint32_t port_id,
                                uint32_t id, void* data, size_t size) {
    auto* self = static_cast<spa_eax_node*>(object);
    if (!self) return -EINVAL;
    (void)port_id;
    (void)size;

    if (id == SPA_IO_Buffers) {
        if (direction == SPA_DIRECTION_INPUT) {
            self->io_in = static_cast<struct spa_io_buffers*>(data);
        } else {
            self->io_out = static_cast<struct spa_io_buffers*>(data);
        }
        return 0;
    }
    return -ENOTSUP;
}

static int eax_node_port_reuse_buffer(void* object, uint32_t port_id, uint32_t buffer_id) {
    (void)object;
    (void)port_id;
    (void)buffer_id;
    return 0;
}

static int eax_node_process(void* object) {
    auto* self = static_cast<spa_eax_node*>(object);
    if (!self) return -EINVAL;

    // Real-Time Processing Callback: Zero malloc, zero locks
    if (self->eax_ctx) {
        eax_context_render_offline(self->eax_ctx, self->rt_scratch, 256, self->sample_rate);
    }
    return SPA_STATUS_HAVE_DATA;
}

static const struct spa_node_methods eax_node_methods = {
    .version = SPA_VERSION_NODE_METHODS,
    .add_listener = eax_node_add_listener,
    .set_callbacks = eax_node_set_callbacks,
    .sync = eax_node_sync,
    .enum_params = eax_node_enum_params,
    .set_param = eax_node_set_param,
    .set_io = eax_node_set_io,
    .send_command = eax_node_send_command,
    .add_port = eax_node_add_port,
    .remove_port = eax_node_remove_port,
    .port_enum_params = eax_node_port_enum_params,
    .port_set_param = eax_node_port_set_param,
    .port_use_buffers = eax_node_port_use_buffers,
    .port_set_io = eax_node_port_set_io,
    .port_reuse_buffer = eax_node_port_reuse_buffer,
    .process = eax_node_process
};

static size_t eax_spa_get_size(const struct spa_handle_factory* factory, const struct spa_dict* params) {
    (void)factory;
    (void)params;
    return sizeof(struct spa_eax_handle);
}

static int eax_spa_init(const struct spa_handle_factory* factory,
                        struct spa_handle* handle,
                        const struct spa_dict* info,
                        const struct spa_support* support,
                        uint32_t n_support) {
    (void)factory;
    (void)info;
    (void)support;
    (void)n_support;

    if (!handle) return -EINVAL;

    auto* h = reinterpret_cast<struct spa_eax_handle*>(handle);
    std::memset(h, 0, sizeof(*h));

    h->node.node.iface = SPA_INTERFACE_INIT(
        SPA_TYPE_INTERFACE_Node,
        SPA_VERSION_NODE,
        &eax_node_methods,
        &h->node
    );
    spa_hook_list_init(&h->node.hooks);
    h->node.sample_rate = 48000;
    eax_context_create_offline(&h->node.eax_ctx, 48000);

    return 0;
}

static int eax_spa_enum_interface_info(const struct spa_handle_factory* factory,
                                       const struct spa_interface_info** info,
                                       uint32_t* index) {
    (void)factory;
    if (!info || !index) return -EINVAL;

    switch (*index) {
    case 0: {
        static const struct spa_interface_info node_info = {
            .type = SPA_TYPE_INTERFACE_Node,
        };
        *info = &node_info;
        (*index)++;
        return 1;
    }
    default:
        return 0;
    }
}

static const struct spa_dict_item eax_spa_info_items[] = {
    { SPA_KEY_FACTORY_AUTHOR, "EAX Native Team" },
    { SPA_KEY_FACTORY_DESCRIPTION, "PipeWire SPA 3D EAX Audio Spatializer Filter Node" },
    { SPA_KEY_FACTORY_USAGE, "DSP Filter" }
};

static const struct spa_dict eax_spa_info_dict = {
    .flags = 0,
    .n_items = SPA_N_ELEMENTS(eax_spa_info_items),
    .items = eax_spa_info_items
};

static const struct spa_handle_factory eax_spa_factory = {
    .version = SPA_VERSION_HANDLE_FACTORY,
    .name = EAX_SPA_FACTORY_NAME,
    .info = &eax_spa_info_dict,
    .get_size = eax_spa_get_size,
    .init = eax_spa_init,
    .enum_interface_info = eax_spa_enum_interface_info
};

extern "C" {

SPA_EXPORT int spa_handle_factory_enum(const struct spa_handle_factory** factory, uint32_t* index) {
    if (!factory || !index) return -EINVAL;
    if (*index == 0) {
        *factory = &eax_spa_factory;
        (*index)++;
        return 1;
    }
    return 0;
}

}
