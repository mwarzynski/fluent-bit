/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <stdlib.h>
#include <fluent-bit.h>
#include <fluent-bit/flb_time.h>
#include "flb_tests_runtime.h"


pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
char *output = NULL;

void set_output(char *val)
{
    pthread_mutex_lock(&result_mutex);
    output = val;
    pthread_mutex_unlock(&result_mutex);
}

char *get_output(void)
{
    char *val;

    pthread_mutex_lock(&result_mutex);
    val = output;
    pthread_mutex_unlock(&result_mutex);

    return val;
}

int callback_test(void* data, size_t size, void* cb_data)
{
    if (size > 0) {
        flb_debug("[test_filter_aws] received message: %s", data);
        set_output(data); /* success */
    }
    return 0;
}


int run_aws_ec2_metadata_server(bool no_tags) {
    int pid = fork();
    if (pid == 0) {
        flb_info("running in child process");
        if (!no_tags) {
            char *args[] = {"bash", "-c", "python3 ../tests/runtime/filter_aws_ec2ws_server.py", NULL};
            execvp(args[0], args);
        } else {
            char *args[] = {"bash", "-c", "python3 ../tests/runtime/filter_aws_ec2ws_server.py no_tags", NULL};
            execvp(args[0], args);
        }
        return 0;
    }
    sleep(1);
    return pid;
}

void flb_test_aws_ec2_tags_present() {
    int server_pid = run_aws_ec2_metadata_server(false);

    int i;
    int ret;
    int bytes;
    char *p = "[0, {\"log\": \"hello, from my ec2 instance\"}]";
    flb_ctx_t *ctx;
    int in_ffd;
    int out_ffd;
    int filter_ffd;
    struct flb_lib_out_cb cb_data;

    ctx = flb_create();

    in_ffd = flb_input(ctx, (char *) "lib", NULL);
    TEST_CHECK(in_ffd >= 0);
    flb_input_set(ctx, in_ffd, "tag", "test", NULL);


    /* Prepare output callback context*/
    cb_data.cb = callback_test;
    cb_data.data = NULL;

    /* Lib output */
    out_ffd = flb_output(ctx, (char *) "lib", (void *)&cb_data);
    TEST_CHECK(out_ffd >= 0);
    flb_output_set(ctx, out_ffd,
                   "match", "*",
                   "format", "json",
                   NULL);

    filter_ffd = flb_filter(ctx, (char *) "aws", NULL);
    TEST_CHECK(filter_ffd >= 0);
    ret = flb_filter_set(ctx, filter_ffd, "match", "*", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "ec2_instance_id", "false", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "az", "false", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "tags_enabled", "true", NULL);
    TEST_CHECK(ret == 0);

    ret = flb_start(ctx);
    TEST_CHECK(ret == 0);

    bytes = flb_lib_push(ctx, in_ffd, p, strlen(p));
    if (!TEST_CHECK(bytes > 0)) {
        TEST_MSG("zero bytes were pushed\n");
    }

    flb_time_msleep(1500);

    char *output = NULL;
    output = get_output();

    char *result = strstr(output, "\"Name\":\"mwarzynski-fluentbit-dev\"");
    if (!TEST_CHECK(result != NULL)) {
        TEST_MSG("output:%s\n", output);
    }
    result = strstr(output, "\"CUSTOMER_ID\":\"70ec5c04-3a6e-11ed-a261-0242ac120002\"");
    if (!TEST_CHECK(result != NULL)) {
        TEST_MSG("output:%s\n", output);
    }
    result = strstr(output, "hello, from my ec2 instance");
    if (!TEST_CHECK(result != NULL)) {
        TEST_MSG("output:%s\n", output);
    }

    flb_stop(ctx);
    flb_destroy(ctx);

    set_output(NULL);
    kill(server_pid, SIGINT);
}


void flb_test_aws_ec2_tags_404() {
    int server_pid = run_aws_ec2_metadata_server(true);

    int i;
    int ret;
    int bytes;
    char *p = "[0, {\"log\": \"hello, from my ec2 instance\"}]";
    flb_ctx_t *ctx;
    int in_ffd;
    int out_ffd;
    int filter_ffd;
    struct flb_lib_out_cb cb_data;

    ctx = flb_create();

    in_ffd = flb_input(ctx, (char *) "lib", NULL);
    TEST_CHECK(in_ffd >= 0);
    flb_input_set(ctx, in_ffd, "tag", "test", NULL);


    /* Prepare output callback context*/
    cb_data.cb = callback_test;
    cb_data.data = NULL;

    /* Lib output */
    out_ffd = flb_output(ctx, (char *) "lib", (void *)&cb_data);
    TEST_CHECK(out_ffd >= 0);
    flb_output_set(ctx, out_ffd,
                   "match", "*",
                   "format", "json",
                   NULL);

    filter_ffd = flb_filter(ctx, (char *) "aws", NULL);
    TEST_CHECK(filter_ffd >= 0);
    ret = flb_filter_set(ctx, filter_ffd, "match", "*", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "ec2_instance_id", "false", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "az", "false", NULL);
    TEST_CHECK(ret == 0);
    ret = flb_filter_set(ctx, filter_ffd, "tags_enabled", "true", NULL);
    TEST_CHECK(ret == 0);

    ret = flb_start(ctx);
    TEST_CHECK(ret == 0);

    bytes = flb_lib_push(ctx, in_ffd, p, strlen(p));
    if (!TEST_CHECK(bytes > 0)) {
        TEST_MSG("zero bytes were pushed\n");
    }

    flb_time_msleep(1500);

    char *output = NULL;
    output = get_output();

    char *result = strstr(output, "\"Name\":\"mwarzynski-fluentbit-dev\"");
    if (!TEST_CHECK(result == NULL)) {
        TEST_MSG("output:%s\n", output);
    }
    result = strstr(output, "\"CUSTOMER_ID\":\"70ec5c04-3a6e-11ed-a261-0242ac120002\"");
    if (!TEST_CHECK(result == NULL)) {
        TEST_MSG("output:%s\n", output);
    }
    result = strstr(output, "hello, from my ec2 instance");
    if (!TEST_CHECK(result != NULL)) {
        TEST_MSG("output:%s\n", output);
    }

    flb_stop(ctx);
    flb_destroy(ctx);

    set_output(NULL);
    kill(server_pid, SIGINT);
}

TEST_LIST = {
    {"aws_ec2_tags_present", flb_test_aws_ec2_tags_present},
    {"aws_ec2_tags_404", flb_test_aws_ec2_tags_404},
    {NULL, NULL}
};
