// socket module for IaiServerless MicroPython port
// Provides standard Python socket interface backed by shim hypercalls
#include "py/runtime.h"
#include "py/obj.h"
#include "../../shim/unistd.h"
#include "../../shim/netdb.h"
#include "../../shim/string.h"
#include <stdint.h>

#define AF_INET     2
#define SOCK_STREAM 1

static inline uint16_t _htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }

struct _sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    uint8_t  sin_zero[8];
};

// --- socket object ---

typedef struct {
    mp_obj_base_t base;
    int fd;
} socket_obj_t;

static const mp_obj_type_t socket_type;

// s.connect((host, port))
static mp_obj_t socket_connect(mp_obj_t self_in, mp_obj_t addr_obj) {
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t *items;
    mp_obj_get_array_fixed_n(addr_obj, 2, &items);
    const char *host = mp_obj_str_get_str(items[0]);
    uint16_t port = (uint16_t)mp_obj_get_int(items[1]);

    struct hostent *he = gethostbyname(host);
    if (!he) mp_raise_OSError(1);

    struct _sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = _htons(port);
    memcpy(&sa.sin_addr, he->h_addr_list[0], 4);

    if (connect(self->fd, &sa, sizeof(sa)) < 0)
        mp_raise_OSError(1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, socket_connect);

// s.send(data)
static mp_obj_t socket_send(mp_obj_t self_in, mp_obj_t data_obj) {
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(data_obj, &buf, MP_BUFFER_READ);
    long n = send(self->fd, buf.buf, buf.len, 0);
    if (n < 0) mp_raise_OSError(1);
    return mp_obj_new_int(n);
}
static MP_DEFINE_CONST_FUN_OBJ_2(socket_send_obj, socket_send);

// s.recv(maxlen) -> bytes or None on EOF
static mp_obj_t socket_recv(mp_obj_t self_in, mp_obj_t len_obj) {
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t maxlen = (size_t)mp_obj_get_int(len_obj);
    if (maxlen > 1024) maxlen = 1024;
    char buf[1024];
    long n = recv(self->fd, buf, maxlen, 0);
    if (n < 0) mp_raise_OSError(1);
    if (n == 0) return mp_const_empty_bytes;
    return mp_obj_new_bytes((const byte *)buf, (size_t)n);
}
static MP_DEFINE_CONST_FUN_OBJ_2(socket_recv_obj, socket_recv);

// s.close()
static mp_obj_t socket_close(mp_obj_t self_in) {
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    close(self->fd);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(socket_close_obj, socket_close);

static const mp_rom_map_elem_t socket_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),    MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),    MP_ROM_PTR(&socket_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),   MP_ROM_PTR(&socket_close_obj) },
};
static MP_DEFINE_CONST_DICT(socket_locals, socket_locals_table);

static mp_obj_t socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)n_args; (void)n_kw; (void)args;
    socket_obj_t *s = mp_obj_malloc(socket_obj_t, type);
    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd < 0) mp_raise_OSError(1);
    return MP_OBJ_FROM_PTR(s);
}

static MP_DEFINE_CONST_OBJ_TYPE(
    socket_type,
    MP_QSTR_socket,
    MP_TYPE_FLAG_NONE,
    make_new, socket_make_new,
    locals_dict, &socket_locals
);

// --- module ---

static const mp_rom_map_elem_t socket_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_socket) },
    { MP_ROM_QSTR(MP_QSTR_socket),      MP_ROM_PTR(&socket_type) },
    { MP_ROM_QSTR(MP_QSTR_AF_INET),     MP_ROM_INT(AF_INET) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM), MP_ROM_INT(SOCK_STREAM) },
};
static MP_DEFINE_CONST_DICT(socket_module_globals, socket_module_globals_table);

const mp_obj_module_t mp_module_socket = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&socket_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_socket, mp_module_socket);
