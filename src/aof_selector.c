#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "hiredis.h"

#define FIELD_TYPE_ALL 0
#define FIELD_TYPE_CMD 1
#define FIELD_TYPE_KEY 2
#define MAX_MATCH_FIELD_LEN 4096 // 4KB
#define PRINT_MODEL_LINE 0
#define PRINT_MODEL_RAW 1

typedef struct SelectorArg {
    int sel_field_idx;
    int where_field_idx;
    char *where_field_vals;
} SelectorArg;

void print_help() {
    printf("usage: aof-selector [-s field_idx] [-w field_idx] [-i field_vals]\n");
    printf("example: cat appendonly.aof | aof-selector -s 0 -w 0 -i set,del\n");
}

int is_match_field(char *wfield_vals, char *field_val, int field_len) {
    char *buf = (char *)malloc(field_len + 3); // ,,\0
    if (buf == NULL) {
        return 0;
    }

    if (wfield_vals == NULL) return 1;
    buf[0] = ',';
    memcpy(buf+1, field_val, field_len);
    buf[field_len + 1] = ',';
    buf[field_len + 2] = '\0';
    int ret = (strstr(wfield_vals, buf) != NULL);
    free(buf);
    return ret;
}

int is_match_condition(SelectorArg *arg, redisReply *reply) {
    if (arg->where_field_idx == -1 || arg->where_field_vals == NULL) {
        return 1;
    }
    if (reply->type != REDIS_REPLY_ARRAY || 
            arg->where_field_idx > (int)reply->elements - 1) {
        return 0;
    }
    redisReply *wfield = reply->element[arg->where_field_idx];
    if (wfield->len > MAX_MATCH_FIELD_LEN) {
        fprintf(stderr, "Match field reach max size:%jd", wfield->len);
        exit(EXIT_FAILURE);
    }
    if (is_match_field(arg->where_field_vals, wfield->str, wfield->len)) {
        return 1;
    } 
    return 0;
}

int print_by_sel_idx(int sel_field_idx, redisReply *reply, int print_model) {
    if (sel_field_idx != -1 && sel_field_idx < (int)reply->elements) {
        redisReply *sel_ele = reply->element[sel_field_idx];
        fwrite(sel_ele->str, sel_ele->len, 1, stdout);
        fprintf(stdout, "\r\n");
    } 
    if (sel_field_idx == -1) { // write all fields
        size_t i = 0;
        if (print_model == PRINT_MODEL_LINE) {
            for (; i < reply->elements; i++) {
                redisReply *ele = reply->element[i];
                fwrite(ele->str, ele->len, 1, stdout);
                fprintf(stdout, " ");
            }
            fprintf(stdout, "\r\n");
        } else {
            fprintf(stdout, "*%jd\r\n", reply->elements);
            for (; i < reply->elements; i++) {
                redisReply *ele = reply->element[i];
                fprintf(stdout, "$%jd\r\n", ele->len);
                fwrite(ele->str, ele->len, 1, stdout);
                fprintf(stdout, "\r\n");
            }
        }
    }
    return 0;
}

int read_reply_loop(SelectorArg *arg, redisReader *reader) {
    int reply_count = 0;
    while (true) {
        redisReply *reply = NULL;
        int ret = redisReaderGetReply(reader, (void **)&reply);
        if (reader->err) {
            fprintf(stderr, "reader parse err:%s\n", reader->errstr);
            return -1;
        }
        if (ret != 0 || reply == NULL) { // not parse over
            break;
        }
        reply_count++;
        if (reply->type == REDIS_REPLY_ERROR) {
            fprintf(stderr, "redis reply err:%s\n", reply->str);
            freeReplyObject(reply);
            return -1;
        }
        if (!is_match_condition(arg, reply)) {
            continue;
        }
        print_by_sel_idx(arg->sel_field_idx, reply, PRINT_MODEL_LINE);
        freeReplyObject(reply);
    }
    return reply_count;
}

int sel_from_stdin(SelectorArg *arg, redisReader *reader) {
    char buf[1024];

    while(1) {
        int nread = read(fileno(stdin), buf, 1024);

        if (nread == 0) { // read over
            break;
        } else if (nread == -1) {
            fprintf(stderr, "Reading from standard input err:%s\n", 
                    strerror(errno));
            return -1;
        }
        int ret = redisReaderFeed(reader, buf, nread);
        if (ret != REDIS_OK) {
            fprintf(stderr, "feed buf err:%.100s\n", buf);
            return -1;
        }
        ret = read_reply_loop(arg, reader);
        if (ret < 0) {
            fprintf(stderr, "get reply err:%s, buf:%.100s\n", reader->errstr, buf);
            return -1;
        }
    }
    return 0;
}

int parse_arg(int argc, char **argv, SelectorArg *sarg) {
    int opt = -1;
    int wfield_vals_len = 0;
    while ((opt = getopt(argc, argv, "hs:w:i:")) != -1) {
        switch (opt) {
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 's':
            sarg->sel_field_idx = atoi(optarg);
            break;
        case 'w':
            sarg->where_field_idx = atoi(optarg);
            break;
        case 'i':
            /* We get the list of tests to run as a string in the form
             * get,set,lrange,...,test_N. Then we add a comma before and
             * after the string in order to make sure that searching
             * for ",testname," will always get a match if the test is
             * enabled. */
            wfield_vals_len = strlen(optarg) + 3; // ,,\0
            sarg->where_field_vals = (char *)malloc(wfield_vals_len);
            snprintf(sarg->where_field_vals, wfield_vals_len, ",%s,", optarg);
            break;
        default:
            print_help();
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    SelectorArg sarg;
    sarg.sel_field_idx = -1;
    sarg.where_field_idx = -1;
    sarg.where_field_vals = NULL;

    int ret = parse_arg(argc, argv, &sarg);
    if (ret != 0) {
        fprintf(stderr, "parse arg err, argc:%d\n", argc);
        return -1;
    }
    redisReader *reader = redisReaderCreate();
    ret = sel_from_stdin(&sarg, reader);

    return ret;
}
