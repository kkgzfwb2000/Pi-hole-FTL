/* Pi-hole: A black hole for Internet advertisements
*  (c) 2021 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  TOML config writer prototypes
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */
#ifndef CONFIG_WRITER_H
#define CONFIG_WRITER_H

#include "FTL.h"
// union conf_value
#include "config.h"
// type toml_table_t
#include "tomlc99/toml.h"

void indentTOML(FILE *fp, const unsigned int indent);
FILE *openFTLtoml(const char *mode) __attribute((malloc)) __attribute((nonnull(1)));
void writeTOMLvalue(FILE * fp, const enum conf_type t, union conf_value *v);
void readTOMLvalue(struct conf_item *conf_item, const char* key, toml_table_t *toml);

#endif //CONFIG_WRITER_H
