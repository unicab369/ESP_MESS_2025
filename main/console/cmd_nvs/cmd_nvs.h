/* Console example — declarations of command registration functions.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once


static struct {
   struct arg_str *key;
   struct arg_str *type;
   struct arg_str *value;
   struct arg_end *end;
} set_args;

static struct {
   struct arg_str *key;
   struct arg_str *type;
   struct arg_end *end;
} get_args;

static struct {
   struct arg_str *key;
   struct arg_end *end;
} erase_args;

static struct {
   struct arg_str *namespace;
   struct arg_end *end;
} erase_all_args;

static struct {
   struct arg_str *namespace;
   struct arg_end *end;
} namespace_args;

static struct {
   struct arg_str *partition;
   struct arg_str *namespace;
   struct arg_str *type;
   struct arg_end *end;
} list_args;


// Register NVS functions
void register_nvs(void);

