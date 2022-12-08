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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>


static t_class *jackconnect_class;

typedef struct _jackconnect
{
    t_object x_obj;
    char source[321];
    char destination[321];
} t_jackconnect;

static jack_client_t *jc;


static void jackconnect_getnames(t_jackconnect *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    char* to = x->source;
    to = (char*)stpcpy( to, output_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, output_port->s_name);
    to = x->destination;
    to = (char*)stpcpy(to, input_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, input_port->s_name);

}

static void jackconnect_connect(t_jackconnect *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        int status;
        int connected = 0;
        jackconnect_getnames(x, output_client, output_port, input_client, input_port);
        logpost(x, 3,
                "[jack-connect] connecting '%s' --> '%s'", x->source, x->destination);
        status = jack_connect(jc, x->source, x->destination);
        if ((!status) || status == EEXIST) connected = 1;
        outlet_float(x->x_obj.ob_outlet, connected);
    }
}

static void jackconnect_disconnect(t_jackconnect *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        jackconnect_getnames(x, output_client, output_port, input_client, input_port);
        jack_disconnect(jc, x->source, x->destination);
        outlet_float(x->x_obj.ob_outlet, 0);
        logpost(x, 3,
                "[jack-connect] disconnecting '%s' --> '%s'", x->source, x->destination);
    }
}

static void jackconnect_query(t_jackconnect *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        const char **ports;
        int n=0;
        jackconnect_getnames(x, output_client, output_port, input_client, input_port);
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
            free(ports);
        }
        outlet_float(x->x_obj.ob_outlet, connected);
    }
}

static void *jackconnect_new(void)
{
    t_jackconnect * x = (t_jackconnect *)pd_new(jackconnect_class);
    outlet_new(&x->x_obj, &s_float);
    return (void*)x;
}

static void setup(void)
{
    jc = jackx_get_jack_client();

    jackconnect_class = class_new(gensym("jack-connect"),
                                  (t_newmethod)jackconnect_new,
                                  0,
                                  sizeof(t_jackconnect),
                                  CLASS_DEFAULT,
                                  0);

    class_addmethod(jackconnect_class, (t_method)jackconnect_connect, gensym("connect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackconnect_class, (t_method)jackconnect_disconnect, gensym("disconnect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackconnect_class, (t_method)jackconnect_query, gensym("query"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
}

void setup_jack0x2dconnect(void)
{
    setup();
}
