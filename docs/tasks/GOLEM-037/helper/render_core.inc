/* Extracted from preserved R24 B module.c, SHA256 8f37b406... .
   Arithmetic/order retained; typed inputs, generated resource IDs and local scratch. */
/* Never retain acquired pointers in persistent module state. */
typedef struct {
    const uint8_t *t[10];
    int lap, count, bin_offset, f, ff, fraction14;
    const RallyRenderStateV1 *state;
    const RallyRenderResourcesV1 *resources;
    RallyRenderContextV1 *context;
    const uint8_t *coeff;
} Assets;
typedef struct {
    uint8_t *p;
    unsigned n, cap;
    int error;
} Out;
static void byte(Out *o, unsigned v) {
    if (o->n < o->cap)
        o->p[o->n++] = v;
    else
        o->error = 1;
}
static void word(Out *o, int v) {
    byte(o, v);
    byte(o, (unsigned)v >> 8);
}
static void command(Out *o, unsigned id, unsigned op) {
    byte(o, 23);
    byte(o, 0);
    byte(o, 160);
    word(o, id);
    byte(o, op);
}
static void end(Out *o) { command(o, 65535, 7); }
static void plot(Out *o, int op, int x, int y) {
    byte(o, 25);
    byte(o, op);
    word(o, x);
    word(o, y);
}
static void colour(Out *o, int c) {
    byte(o, 18);
    byte(o, 0);
    byte(o, c);
}
static void rect(Out *o, int x, int y, int xx, int yy, int c) {
    colour(o, c);
    plot(o, 4, x, y);
    plot(o, 101, xx, yy);
}
static void viewport(Out *o, int top) {
    byte(o, 24);
    word(o, 0);
    word(o, 223);
    word(o, 319);
    word(o, top);
    plot(o, 4, 0, top);
}
static void affine(Out *o, int id) {
    byte(o, 23);
    byte(o, 0);
    byte(o, 150);
    byte(o, 1);
    word(o, id);
}
static void bitmap(Out *o, int b, int matrix, int x, int y) {
    byte(o, 23);
    byte(o, 27);
    byte(o, 0);
    byte(o, b);
    affine(o, matrix);
    byte(o, 23);
    byte(o, 27);
    byte(o, 3);
    word(o, x);
    word(o, y);
}
static void matrix(Out *o, int id, int xx, int scale, int translation) {
    command(o, id, 32);
    byte(o, 0);
    command(o, id, 32);
    byte(o, 11);
    byte(o, 200);
    word(o, xx);
    word(o, 0);
    word(o, translation);
    word(o, 0);
    word(o, scale);
    word(o, 0);
}
/* FractionQ14 cannot be reconstructed from Q12: keep its two low bits. */
static int centre_exact(const Assets *a, int y, int fraction14) {
    if (y < 103 || y > 224)
        return 40960;
    int q = y - 96, k = (fraction14 + s32(a->t[7] + q * 4)) >> 14;
    const uint8_t *z = a->coeff + k * 12;
    int aa = s16(z) * 4 + s16(z + 2) * a->f / 4096;
    int bb = s32(z + 4) + s16(z + 8) * a->f / 4096 + s16(z + 10) * a->ff / 4096;
    return aa + bb * q / 16;
}
static int cx(const Assets *a, int y) { return centre_exact(a, y, a->fraction14) / 256; }
static int nearest(int n) { return n >= 0 ? (n + 128) / 256 : -((-n + 128) / 256); }
static void section(Out *o, int ct, int cb, int yt, int yb, int marked) {

    /* Preserve binary32 matrix order; conversion is explicit signed truncation.
       Physical DSP accumulator parity remains a later qualification gate. */
    float a = (float)yt * (1.0f / 50.0f) + (-96.0f / 50.0f);
    float b = (float)(cb - ct), c = (float)yt * (-1.0f / 50.0f) + (float)yb * (1.0f / 50.0f);
    static const int left[5] = {-106, -90, -86, 84, -2}, right[5] = {106, 90, -84, 86, 2};
    for (int i = 0; i < (marked ? 5 : 4); ++i) {
        colour(o, i == 0 ? (marked ? 9 : 15) : i == 1 ? 8 : (marked ? 11 : 15));
        for (int v = 0; v < 2; ++v)
            for (int side = 0; side < 2; ++side) {
                int u = side ? right[i] : left[i];
                float x = a * (float)u;
                x = x + b * (float)v;
                x = x + c * (float)(u * v);
                x = x + (float)ct;
                plot(o, v ? 85 : 4, (int)x, v ? yb : yt);
            }
    }

}
static int ahead(int car, int camera, int lap) {
    int d = car - camera;
    return d < 0 ? d + lap : d;
}
static unsigned road(Out *o, const Assets *a) {
    int pos = a->state->player_station, phase = a->state->phase, first = phase < 4000;
    const uint8_t *stripe = a->t[6] + u16(a->t[5] + a->t[4][phase % 4000] * 2);
    const uint8_t *curve = a->t[3] + u16(a->t[2] + (pos / 1600 + a->bin_offset) * 2);
    int y = 104, ct = nearest(centre_exact(a, y, a->fraction14)), marked = 0;
    unsigned bands = 0;
    rect(o, 0, 104, 319, 223, 2);
    while (y < 224 && bands < 120) {
        int yc = *curve + 1, ys = *stripe, yy = yc < ys ? yc : ys;
        if (yy <= y || yy > 224) {
            o->error = 1;
            break;
        }
        int cb = nearest(centre_exact(a, yy, a->fraction14));
        section(o, ct, cb, y, yy, marked);
        ++bands;
        if (yy == yc)
            ++curve;
        if (yy == ys) {
            marked = yy == 116 ? first : !marked;
            ++stripe;
        }
        y = yy;
        ct = cb;
    }
    rect(o, 0, 224, 319, 239, 0);
    int z = ahead(0, pos, a->lap) / 100;
    if (z >= 64 && z <= 1000) {
        int yy = 96 + 8000 / z, h = (yy - 96) / 16;
        if (h < 1)
            h = 1;
        if (yy >= 104 && yy <= 223)
            for (int row = 0; row < 2; ++row) {
                int r = yy + row * h;
                if (r > 223)
                    break;
                int c = cx(a, r), half = (r - 96) * 90 / 50, bottom = r + h - 1;
                if (bottom > 223)
                    bottom = 223;
                for (int j = 0; j < 12; ++j)
                    rect(o, c - half + half * 2 * j / 12, r, c - half + half * 2 * (j + 1) / 12 - 1,
                         bottom, ((j + row) & 1) ? 15 : 0);
            }
    }
    end(o);
    return bands;
}
static int tangent(const Assets *a, int pos, int axis) {
    int d = (pos / 100) * 256, index = d >> 14, f = d & 16383,
        next = index + 1 == a->count ? 0 : index + 1;
    int x = s16(a->t[1] + index * 8 + axis), y = s16(a->t[1] + next * 8 + axis);
    return x + ((y - x) * f >> 14);
}
static int bearing(const Assets *a, int center) {
    int x = center - 160, neg = x < 0;
    if (neg)
        x = -x;
    if (x > 512)
        x = 512;
    int i = x >> 1, v = s16(a->t[8] + i * 2);
    if (x & 1)
        v += (s16(a->t[8] + (i + 1) * 2) - v + 1) / 2;
    return neg ? -v : v;
}
static int heading(const Assets *a, int cross) {
    int neg = cross < 0;
    if (neg)
        cross = -cross;
    if (cross > 4096)
        cross = 4096;
    int i, f, s;
    if (cross <= 3584) {
        i = cross >> 5;
        f = cross & 31;
        s = 5;
    } else if (cross <= 3968) {
        i = 112 + ((cross - 3584) >> 3);
        f = (cross - 3584) & 7;
        s = 3;
    } else {
        i = 160 + cross - 3968;
        f = 0;
        s = 0;
    }
    int v = s16(a->t[9] + i * 2);
    if (f)
        v += ((s16(a->t[9] + (i + 1) * 2) - v) * f) >> s;
    return neg ? -v : v;
}
typedef RallyRenderActorScratchV1 Car;
static int view(int yaw) {
    if (yaw < 0)
        yaw = -yaw;
    int v = (yaw + 672) / 1344;
    return v > 4 ? 4 : v;
}
static unsigned actors(Out *o, const Assets *a, unsigned *sign_count) {
    Car *cars=a->context->cars, *signs=a->context->signs;
    int32_t *order=a->context->order, *distance=a->context->distance;
    int pos = a->state->player_station, flags = a->state->flags, n = 0, ns = 0;
    for (int i = 0; i < 6; ++i) {
        order[i] = i;
        distance[i] = a->state->opponents[i].active ? ahead(a->state->opponents[i].station, pos, a->lap) : 0;
    }
    for (int i = 0; i < 5; ++i)
        for (int j = i + 1; j < 6; ++j)
            if (distance[order[i]] < distance[order[j]]) {
                int t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
    int tx = tangent(a, pos, 4), ty = tangent(a, pos, 6);
    for (int j = 0; j < 6; ++j) {
        int i = order[j];
        const RallyRenderOpponentV1 *p = &a->state->opponents[i];
        int z = distance[i] / 100;
        if (!(flags & 1) || !p->active || z < 64 || z > 1100)
            continue;
        int q = 8000 / z, y = 96 + q, c = cx(a, y) + (p->lateral_q8 / 256) * q / 50, scale = q * 2;
        int cross = (tx * tangent(a, p->station, 6) - ty * tangent(a, p->station, 4)) / 4096;
        int yaw = heading(a, cross) + ((i & 1) ? -1 : 1) * p->heading_turn16;
        if (flags & 4)
            yaw -= bearing(a, c);
        Car r = {a->resources->traffic_bitmap_base + i * 5 + view(yaw),
                 a->resources->traffic_matrix_base + i,
                 scale,
                 yaw > 0 ? -scale : scale,
                 yaw > 0 ? 101 * scale : 0,
                 c - 51 * scale / 256,
                 y - 70 * scale / 256};
        cars[n++] = r;
    }
    int total = a->lap / 48000, near = pos * total / a->lap;
    for (int k = -1; k <= 2; ++k) {
        int i = (near + k + total) % total, station = (i * a->lap / total + 24000) % a->lap,
            z = ahead(station, pos, a->lap) / 100;
        if (z < 64 || z > 500)
            continue;
        int q = 8000 / z, ground = 96 + q, scale = q * 2, y = ground - 64 * scale / 256;
        if (y < 104 || ground > 223)
            continue;
        int x = cx(a, ground) + ((i & 1) ? -1 : 1) * 134 * q / 50 - 32 * scale / 256;
        if (x >= 320 || x + 64 * scale / 256 <= 0)
            continue;
        if (ns < 2) {
            Car r = {a->resources->sign_bitmap_base + i % 8, a->resources->sign_matrix, scale, scale, 0, x, y};
            signs[ns++] = r;
        }
    }
    if (ns == 2 && signs[0].scale > signs[1].scale) {
        Car t = signs[0];
        signs[0] = signs[1];
        signs[1] = t;
    }
    int c = 0, s = 0;
    while (c < n || s < ns) {
        int sign = s < ns && (c == n || signs[s].scale <= cars[c].scale);
        Car *r = sign ? &signs[s++] : &cars[c++];
        if (sign)
            viewport(o, 104);
        matrix(o, r->matrix, r->xx, r->scale, r->translation);
        bitmap(o, r->bitmap, r->matrix, r->x, r->y);
        if (sign)
            viewport(o, flags & 2 ? 88 : 24);
    }
    affine(o, 65535);
    end(o);
    *sign_count = ns;
    return n;
}
static void player(Out *o, const Assets *a) {
    int c = cx(a, 214) + a->state->player_lateral_q8 * 118 / (50 * 256), remaining = a->state->reference_crash_remaining,
        inv = a->state->reference_invulnerable, steering = a->state->steering;
    if (remaining) {
        int age = 150 - remaining, radius = 8 + (age < 60 ? age / 3 : 20 - (age - 60) / 5);
        if (radius < 4)
            radius = 4;
        const int8_t dx[8] = {0, 7, 10, 7, 0, -7, -10, -7}, dy[8] = {-10, -7, 0, 7, 10, 7, 0, -7};
        for (int i = 0; i < 8; ++i) {
            int x = c + dx[i] * radius / 10, y = 193 + dy[i] * radius / 10,
                size = 3 + (i + age / 8) % 6;
            rect(o, x - size, y - size, x + size, y + size,
                 (i + age / 6) % 3 == 0 ? 15 : ((i & 1) ? 11 : 9));
        }
        rect(o, c - 6, 187, c + 6, 199, 15);
    } else if (!inv || !((inv / 10) & 1)) {
        int yaw = steering * 256 - bearing(a, c), mirror, v;
        if (a->state->flags & 4) {
            v = view(yaw);
            mirror = yaw > 0;
        } else {
            mirror = steering > 0;
            v = ((steering < 0 ? -steering : steering) * 4 + 10) / 21;
        }
        bitmap(o, a->resources->player_bitmap_base + v, mirror ? a->resources->player_mirror_matrix : 65535, c - (mirror ? 50 : 51), 154);
        affine(o, 65535);
    }
    end(o);
}
