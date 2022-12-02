/* jack utility externals for linux pd
 * copyright 2003 Gerard van Dongen gml@xs4all.nl
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 * This provides a function to make sure all instances of jackx
 * objects are using the same jack connection.

 */

#include "m_pd.h"
#include <jack/jack.h>

static jack_client_t *jc;

jack_client_t * jackx_get_jack_client()
{
    if (!jc)
    {
        jack_status_t status;
        jc = jack_client_open ("jackx-pd", JackNullOption, &status, NULL);
        if (status & JackServerFailed) {
            pd_error(NULL, "jackx: unable to connect to JACK server");
            jc = NULL;
        }
        if (status) {
            if (status & JackServerStarted) {
                verbose(1, "jackx: started server");
            } else {
                pd_error(NULL, "jackx: server returned status %d", status);
            }
        }
    }
    return jc;
}
