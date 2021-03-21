#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <stlink.h>

static void usage(void) {
    puts("st-info --version");
    puts("st-info --probe");
    puts("st-info --serial");
    puts("st-info --flash  [--connect-under-reset]");
    puts("st-info --pagesize  [--connect-under-reset]");
    puts("st-info --sram  [--connect-under-reset]");
    puts("st-info --chipid  [--connect-under-reset]");
    puts("st-info --descr  [--connect-under-reset]");
}

static void stlink_print_version(stlink_t *sl) {
    // Implementation of version printing is minimalistic
    // but contains all available information from sl->version
    printf("V%u", sl->version.stlink_v);
    if (sl->version.jtag_v > 0)
        printf("J%u", sl->version.jtag_v);
    if (sl->version.swim_v > 0)
        printf("S%u", sl->version.swim_v);
    printf("\n");
}

static void stlink_print_info(stlink_t *sl) {
    const struct stlink_chipid_params *params = NULL;

    if (!sl) { return; }

    printf("  version:    ");
    stlink_print_version(sl);
    printf("  serial:     %s\n", sl->serial);
    printf("  flash:      %u (pagesize: %u)\n",
           (uint32_t)sl->flash_size, (uint32_t)sl->flash_pgsz);
    printf("  sram:       %u\n", (uint32_t)sl->sram_size);
    printf("  chipid:     0x%.4x\n", sl->chip_id);

    params = stlink_chipid_get_params(sl->chip_id);

    if (params) { printf("  descr:      %s\n", params->description); }
}

static void stlink_probe(void) {
    stlink_t **stdevs;
    size_t size;

    size = stlink_probe_usb(&stdevs);

    printf("Found %u stlink programmers\n", (unsigned int)size);

    for (size_t n = 0; n < size; n++) {
        if (size > 1) printf("%u.\n", (unsigned int)n+1);
        stlink_print_info(stdevs[n]);
    }

    stlink_probe_usb_free(&stdevs, size);
}

static stlink_t *stlink_open_first(bool under_reset) {
    stlink_t* sl = NULL;
    sl = stlink_v1_open(0, 1);

    if (sl == NULL) {
        if (under_reset) {
            sl = stlink_open_usb(0, 2, NULL, 0);
        } else {
            sl = stlink_open_usb(0, 1, NULL, 0);
        }
    }

    return(sl);
}

static int print_data(int ac, char **av) {
    stlink_t* sl = NULL;
    bool under_reset = false;

    // probe needs all devices unclaimed
    if (strcmp(av[1], "--probe") == 0) {
        stlink_probe();
        return(0);
    } else if (strcmp(av[1], "--version") == 0) {
        printf("v%s\n", STLINK_VERSION);
        return(0);
    }

    if (ac == 3) {
        if (strcmp(av[2], "--connect-under-reset") == 0) {
            under_reset = true;
        } else {
            usage();
            return(-1);
        }
    }

    sl = stlink_open_first(under_reset);

    if (sl == NULL) { return(-1); }

    sl->verbose = 0;

    if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE) { stlink_exit_dfu_mode(sl); }

    if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE) { stlink_enter_swd_mode(sl); }

    if (strcmp(av[1], "--serial") == 0) {
        printf("%s\n", sl->serial);
    } else if (strcmp(av[1], "--flash") == 0) {
        printf("0x%x\n", (uint32_t)sl->flash_size);
    } else if (strcmp(av[1], "--pagesize") == 0) {
        printf("0x%x\n", (uint32_t)sl->flash_pgsz);
    } else if (strcmp(av[1], "--sram") == 0) {
        printf("0x%x\n", (uint32_t)sl->sram_size);
    } else if (strcmp(av[1], "--chipid") == 0) {
        printf("0x%.4x\n", sl->chip_id);
    } else if (strcmp(av[1], "--descr") == 0) {
        const struct stlink_chipid_params *params = stlink_chipid_get_params(sl->chip_id);

        if (params == NULL) { return(-1); }

        printf("%s\n", params->description);
    }

    if (sl) {
        stlink_exit_debug_mode(sl);
        stlink_close(sl);
    }

    return(0);
}

int main(int ac, char** av) {
    int err = -1;

    if (ac < 2) {
        usage();
        return(-1);
    }

    err = print_data(ac, av);

    return(err);
}