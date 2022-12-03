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
 *
 * jack-connect
 * this can query and set the port connections on the jack system
 */

#include "jackx.h"
#include "m_pd.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <jack/jack.h>


static t_class *jackconnect_class;

typedef struct _jackconnect
{
    t_object x_obj;
    t_symbol *input_client,*input_port, *output_client,*output_port;
    char source[128],destination[128]; //ought to be enough for most names
    int connected;
} t_jackconnect;

static jack_client_t *jc;


static void jackconnect_getnames(t_jackconnect *x)
{
    char* to = x->source;
    to = (char*)stpcpy( to, x->output_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, x->output_port->s_name);
    to = x->destination;
    to = (char*)stpcpy(to, x->input_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, x->input_port->s_name);

}

static void jackconnect_connect(t_jackconnect *x)
{
    if (jc)
    {
        jackconnect_getnames(x);
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43
        // Pd < 0.43 doesn't have logpost()
        post(
#else
        logpost(x, 3,
#endif
                "[jack-connect] connecting '%s' --> '%s'", x->source, x->destination);
        if (!jack_connect(jc, x->source, x->destination))
        {
            x->connected = 1;
            outlet_float(x->x_obj.ob_outlet, x->connected);
        }
    }
}

static void jackconnect_disconnect(t_jackconnect *x)
{
    if (jc)
    {
        jackconnect_getnames(x);
        if (!jack_disconnect(jc, x->source, x->destination))
        {
            x->connected = 0;
            outlet_float(x->x_obj.ob_outlet, x->connected);
        }
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43
        // Pd < 0.43 doesn't have logpost()
        post(
#else
        logpost(x, 3,
#endif
                "[jack-connect] disconnecting '%s' --> '%s'", x->source, x->destination);
    }
}

static void jackconnect_toggle(t_jackconnect *x)
{
    if (jc)
    {
        jackconnect_getnames(x);
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43
        // Pd < 0.43 doesn't have logpost()
        post(
#else
        logpost(x, 3,
#endif
                "[jack-connect] toggling connection '%s' --> '%s'", x->source, x->destination);
        if (jack_disconnect(jc, x->source, x->destination))
        {
            jack_connect(jc, x->source, x->destination);
            x->connected = 1;
        }
        else
        {
            x->connected = 0;
        }
        outlet_float(x->x_obj.ob_outlet, x->connected);
    }
}

static void jackconnect_query(t_jackconnect *x)
{
    if (jc)
    {
        const char **ports;
        int n=0;
        jackconnect_getnames(x);
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43
        // Pd < 0.43 doesn't have logpost()
        post(
#else
        logpost(x, 3,
#endif
                "[jack-connect] querying connection '%s' --> '%s'", x->source, x->destination);

        ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc, x->source));
        x->connected = 0;

        if(ports)
        {
            while (ports[n])
            {
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43
                // Pd < 0.43 doesn't have logpost()
                post(
#else 
                logpost(x, 4,
#endif
                        "n = %i", n);
                if (!strcmp(ports[n], x->destination))
                {
                    x->connected = 1;
                    break;
                }
                n++;

            }
            free(ports);
        }
        outlet_float(x->x_obj.ob_outlet, x->connected);
    }
}

static void *jackconnect_new(t_symbol *output_client, t_symbol *output_port,
                             t_symbol *input_client, t_symbol *input_port)
{
    t_jackconnect * x = (t_jackconnect *)pd_new(jackconnect_class);

    outlet_new(&x->x_obj, &s_float);
    symbolinlet_new(&x->x_obj, &x->output_client);
    symbolinlet_new(&x->x_obj, &x->output_port);
    symbolinlet_new(&x->x_obj, &x->input_client);
    symbolinlet_new(&x->x_obj, &x->input_port);

    /* to prevent segfaults put default names in the client/port variables */
    x->input_client = input_client;
    x->input_port = input_port;
    x->output_client = output_client;
    x->output_port = output_port;
    x->connected = 0;
    jackconnect_getnames(x);

    return (void*)x;
}

void jack0x2dconnect_setup(void)
{
    jc = jackx_get_jack_client();

    jackconnect_class = class_new(gensym("jack-connect"),
                                  (t_newmethod)jackconnect_new,
                                  0,
                                  sizeof(t_jackconnect),
                                  CLASS_DEFAULT,
                                  A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL,
                                  0);

    class_addmethod(jackconnect_class, (t_method)jackconnect_connect, gensym("connect"),0);
    class_addmethod(jackconnect_class, (t_method)jackconnect_disconnect, gensym("disconnect"),0);
    class_addmethod(jackconnect_class, (t_method)jackconnect_toggle, gensym("toggle"),0);
    class_addmethod(jackconnect_class, (t_method)jackconnect_query, gensym("query"),0);
    class_addbang(jackconnect_class, (t_method)jackconnect_toggle);
}

