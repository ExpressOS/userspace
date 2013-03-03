/*
 * Copyright (c) 2012-2013 University of Illinois at
 * Urbana-Champaign. All rights reserved.
 *
 * Developed by:
 *
 *     Haohui Mai
 *     University of Illinois at Urbana-Champaign
 *     http://haohui.me
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal with the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimers.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimers in the documentation and/or other materials
 *      provided with the distribution.
 *
 *    * Neither the names of University of Illinois at
 *      Urbana-Champaign, nor the names of its contributors may be
 *      used to endorse or promote products derived from this Software
 *      without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 */

#define LOG_TAG "SQLEval"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <sqlite3.h>
#include <errno.h>
#include <cutils/log.h>

static void preload_code(void)
{
	extern char *_start, *_end;
	unsigned long start = (unsigned long)&_start;
	unsigned long end = (unsigned long)&_end;
	while (start < end) {
		*(volatile unsigned int*)start;
		start += 4096;
	}
}

static void num_to_str(int num, char *buf) {
        char *cursor = buf;
        static const char *num_char[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
        int digits[10];

        int i = 0;
        do {
                int digit = num % 10;
                num /= 10;
                digits[i++] = digit;
        } while (num > 0);


        for (i--; i >= 0; --i) {
                sprintf(cursor, i ? "%s " : "%s", num_char[digits[i]]);
                cursor += strlen(num_char[digits[i]]) + 1;
        }
}

static void check_return(sqlite3 *db, int ret)
{
        if (ret != 0 && ret != SQLITE_DONE) {
                printf("Failed to execute statement, ret=%d, errno=%d, '%s'\n", ret, errno, sqlite3_errmsg(db));
                abort();
        } }

static void test_insert(sqlite3 *db)
{
        static const char sql_prepare[] = "BEGIN TRANSACTION;";
        static const char sql_finalize[] = "COMMIT TRANSACTION;";
        char buf1[512], buf2[512];
        char *err_msg;
        int ret;
        sqlite3_stmt *begin_stmt = NULL;

        ret = sqlite3_prepare_v2(db, sql_prepare, -1, &begin_stmt, NULL);
        check_return(db, ret);
        
        ret = sqlite3_step(begin_stmt);
        check_return(db, ret);

        ret = sqlite3_reset(begin_stmt);
        check_return(db, ret);


        int i = 0;
        for (i = 0; i < 25000; ++i) {
                static const int period = 32768;
                int r = random() % period;
                r = r + period / 2;
                num_to_str(r, buf1);
                sqlite3_stmt *stmt;
                sprintf(buf2, "INSERT INTO t1 VALUES(%d,%d,'%s');", i, r, buf1);
                ret = sqlite3_prepare_v2(db, buf2, -1, &stmt, NULL);
                check_return(db, ret);

                ret = sqlite3_step(stmt);
                check_return(db, ret);

                ret = sqlite3_reset(stmt);
                check_return(db, ret);
        }

        ret = sqlite3_exec(db, sql_finalize, NULL, NULL, &err_msg);
        if (ret) {
                printf("Finalize failed '%s'\n", err_msg);
                return;
        }
}

static sqlite3 *prepare_db(const char *filename)
{
        sqlite3 *db = NULL;
        char *err_msg;
        
        int ret = sqlite3_open(filename, &db);
        if (ret) {
                printf("Cannot open db %s\n", filename);
                return NULL;
        }

        sqlite3_extended_result_codes(db, 1);
        
        const char sqls[] = { "PRAGMA default_cache_size=2000;"
                              "PRAGMA default_synchronous=off;"
                              "PRAGMA journal_mode = WAL;"
                              "PRAGMA wal_autocheckpoint = 4096;"
                              "PRAGMA locking_mode = EXCLUSIVE;"
                              "DROP TABLE IF EXISTS t1;"
                              "CREATE TABLE t1(a INTEGER, b INTEGER, c VARCHAR(100));"
        };

        ret = sqlite3_exec(db, sqls, NULL, NULL, &err_msg);
        check_return(db, ret);
        
        return db;
}

int main(int argc, char *argv[]) {
	preload_code();
        struct timeval tv_start, tv_end;
        gettimeofday(&tv_start, NULL);
        
        sqlite3 *db = prepare_db(argv[1]);
        if (!db)
                return 0;

        test_insert(db);
        sqlite3_close(db);

        gettimeofday(&tv_end, NULL);
        LOGI("time: %ld ms\n", (tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000);
        return 0;
}

