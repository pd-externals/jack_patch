/* jack utility externals for linux pd
 * copyright 2003 Gerard van Dongen gml@xs4all.nl
 * copyright 2022 Roman Haefeli <reduzent@gmail.com>
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
 *
 * jack_patch
 * this can query and set the port connections on the jack system
 */

#include "m_pd.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#define CLASS_NAME "jack_patch"

static t_class *jackpatch_class;

typedef struct _jackpatch
{
    t_object x_obj;
    char source[321];
    char destination[321];
} t_jackpatch;

static jack_client_t *jc;

jack_client_t * jackx_get_jack_client()
{
    if (!jc)
    {
        jack_status_t status;
        jack_options_t options = JackNoStartServer;
        jc = jack_client_open ("jackx-pd", options, &status, NULL);
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

int jackpatch_getnames(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    // check if all arguments have been specified
    if(!strcmp(input_port->s_name, "")
        || !strcmp(input_client->s_name, "")
        || !strcmp(output_port->s_name, "")
        || !strcmp(output_client->s_name, ""))
    {
        logpost(x, 1, "%s: not enough arguments given", CLASS_NAME);
        return 1;
    }
    char* to = x->source;
    to = (char*)stpcpy( to, output_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, output_port->s_name);
    to = x->destination;
    to = (char*)stpcpy(to, input_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, input_port->s_name);
    return 0;
}

void jackpatch_connect(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        int status;
        int connected = 0;
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        logpost(x, 3,
                "[%s] connecting '%s' --> '%s'", CLASS_NAME,  x->source, x->destination);
        status = jack_connect(jc, x->source, x->destination);
        if ((!status) || status == EEXIST) connected = 1;
        outlet_float(x->x_obj.ob_outlet, connected);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_disconnect(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        jack_disconnect(jc, x->source, x->destination);
        outlet_float(x->x_obj.ob_outlet, 0);
        logpost(x, 3,
                "[%s] disconnecting '%s' --> '%s'", CLASS_NAME, x->source, x->destination);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_query(t_jackpatch *x,
                    t_symbol *output_client, t_symbol *output_port,
                    t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        const char **ports;
        int n=0;
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        logpost(x, 3,
                "[jack-connect] querying connection '%s' --> '%s'", x->source, x->destination);

        ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc, x->source));
        int connected = 0;

        if(ports)
        {
            while (ports[n])
            {
                logpost(x, 4,
                        "n = %i", n);
                if (!strcmp(ports[n], x->destination))
                {
                    connected = 1;
                    break;
                }
                n++;

            }
            jack_free(ports);
        }
        outlet_float(x->x_obj.ob_outlet, connected);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void *jackpatch_new(void)
{
    t_jackpatch * x = (t_jackpatch *)pd_new(jackpatch_class);
    outlet_new(&x->x_obj, &s_float);
    return (void*)x;
}

void jack_patch_setup(void)
{
    jc = jackx_get_jack_client();

    jackpatch_class = class_new(gensym(CLASS_NAME), (t_newmethod)jackpatch_new,
                                  0, sizeof(t_jackpatch), CLASS_DEFAULT, 0);

    class_addmethod(jackpatch_class, (t_method)jackpatch_connect, gensym("connect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_disconnect, gensym("disconnect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_query, gensym("query"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
}
