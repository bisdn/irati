/*
 * Shim related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINA_SHIM_UTILS_H
#define RINA_SHIM_UTILS_H

#include "common.h"

/*
 * Allocates a new name, returning the allocated object. In case of an error, a
 * NULL is returned.
 */
struct name * name_create(void);

/*
 * Initializes a previously dynamically allocated name (i.e. name_alloc())
 * or a statically one (e.g. declared into a struct not as a pointer).
 * Returns the passed object pointer  in case everything is ok, a NULL
 * otherwise. In case of error a call to name_free() is allowed in order to
 * release the associated resources.
 * It is allowed to call name_init() over an already initialized object
 */
struct name * name_init(struct name *    dst,
                        const string_t * process_name,
                        const string_t * process_instance,
                        const string_t * entity_name,
                        const string_t * entity_instance);

/*
 * Finalize a name object, releasing all the embedded resources (without
 * releasing the object itself). After name_fini() execution the passed
 * object will be in the same states as at the end of name_init().
 */
void          name_fini(struct name * dst);

/* This function performs as name_alloc() and name_init() */
struct name * name_alloc_and_init(const string_t * process_name,
                                  const string_t * process_instance,
                                  const string_t * entity_name,
                                  const string_t * entity_instance);

/* Releases all the associated resources bound to a name object */
void          name_destroy(struct name * ptr);

/* Duplicates a name object, returning the pointer to the new object */
struct name * name_dup(const struct name * src);

/*
 * Copies the source object contents into the destination object, both must
 * be previously allocated. Returns 
 */
int           name_cpy(const struct name * src, struct name * dst);
#endif
