// Copyright (C) 2024  Paul Johnson
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <Server/Client.h>
#include <stddef.h>

#ifdef RR_WORKER_MODE

void rr_worker_storage_save_account(char *uuid, struct rr_server_client *client);
int rr_worker_storage_load_account(char *uuid, struct rr_server_client *client);
void rr_worker_storage_set_server_alias(char *alias);
void rr_worker_storage_get_server_alias(char *alias, size_t size);

#endif // RR_WORKER_MODE

