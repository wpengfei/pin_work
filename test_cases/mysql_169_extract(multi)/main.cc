// Copyright 2011 The University of Michigan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors - Jie Yu (jieyu@umich.edu)
// Modified by Pengfei Wang(wpengfeinudt@gmail.com)

#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

class MYSQL{
 public:
  MYSQL() {
    pthread_mutex_init(&mutex_, NULL);
  }

  ~MYSQL() {}

  void write_log(const char *content) {
    log_contents_.push_back(content);
  }

  const char *get_log(int i) {
    return log_contents_[i];
  }

  void insert_table_entry(int key, int val) {
    //pthread_mutex_lock(&mutex_);
    Lock();
    table_contents_[key] = val;
    //pthread_mutex_unlock(&mutex_);
    Unlock();
  }

  void remove_table_entries() {
    //pthread_mutex_lock(&mutex_);
    Lock();
    table_contents_.clear();
    //pthread_mutex_unlock(&mutex_);
    Unlock();
  }

  bool is_table_empty() {
    return table_contents_.empty();
  }

 private:
  void Lock() { pthread_mutex_lock(&mutex_); }
  void Unlock() { pthread_mutex_unlock(&mutex_); }

  pthread_mutex_t mutex_;
  std::vector<const char *> log_contents_;
  std::map<int, int> table_contents_;
};


MYSQL mysql;

/* A typical multi-variable atomicity violation.
Since table_content and log_content are semantically connected,
operations on the table and log should be atomic.

Buggy interleaving:
      T1                                         T2
 mysql.remove_table_entries(); 

                                        mysql.insert_table_entry(1, 2);
                                        mysql.write_log("insert");

 mysql.write_log("remove");

*/

void *delete_thread_main(void *args) {
  printf("removing\n");
  mysql.remove_table_entries();
  mysql.write_log("remove");
  printf("removing done\n");
  return NULL;
}

void *insert_thread_main(void *args) {
  printf("inserting\n");
  mysql.insert_table_entry(1, 2);
  mysql.write_log("insert");
  printf("inserting done\n");
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t delete_tid;
  pthread_t insert_tid;
  pthread_create(&delete_tid, NULL, delete_thread_main, NULL);
  pthread_create(&insert_tid, NULL, insert_thread_main, NULL);
  pthread_join(delete_tid, NULL);
  pthread_join(insert_tid, NULL);

  // validate results
  if (mysql.is_table_empty()) {
    assert(!strcmp(mysql.get_log(0), "insert"));
  } else {
    assert(!strcmp(mysql.get_log(0), "remove"));
  }

  printf("Program exit normally\n");

  return 0;
}

