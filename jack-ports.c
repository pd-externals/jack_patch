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
 * jack-ports
 * this can query jack ports with regex's
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


static t_class *jackports_class;

typedef struct _jackports
{
    t_object x_obj;
    t_outlet *input_ports, *output_ports;
    char expression[128];
    char *buffer; //used internally it doesn't have to be reserved every time
    t_atom *a_outlist;

} t_jackports;

static jack_client_t * jc;


static void jackports_input(t_jackports *x, t_symbol *s,int argc, t_atom *argv)
{
    if (jc)
    {
        const char ** ports;

        int l = 0;
        int n = 0;
        int keyflag = 0;
        int expflag =0;
        int portflags = 0;
        t_symbol *s_client;
        t_symbol *s_port;
        char *t;

        if (!strcmp(s->s_name,"bang"))
        {
            strcpy(x->expression,"");
            expflag = 1;
        }
        else
        {

            //parse symbol s and all arguments for keywords:
            //physical,virtual,input and output

            if (!strcmp(s->s_name,"physical"))
            {
                portflags = portflags | JackPortIsPhysical;
                keyflag = 1;
            }
            if (!strcmp(s->s_name,"virtual"))
            {
                portflags = portflags & (~JackPortIsPhysical);
                keyflag = 1;
            }
            if (!strcmp(s->s_name,"input"))
            {
                portflags = portflags | JackPortIsInput;
                keyflag = 1;
            }

            if (!strcmp(s->s_name,"output"))
            {
                portflags = portflags | JackPortIsOutput;
                keyflag = 1;
            }
            if (!keyflag)
            {
                strcpy(x->expression,s->s_name);
                expflag = 1;
            }
            for (n=0; n<argc; n++)
            {
                keyflag = 0;
                atom_string(argv+n, x->buffer,128);
                if (!strcmp(x->buffer,"physical"))
                {
                    portflags = portflags | JackPortIsPhysical;
                    keyflag = 1;
                }
                if (!strcmp(x->buffer,"virtual"))
                {
                    portflags = portflags & (~JackPortIsPhysical);
                    keyflag = 1;
                }
                if (!strcmp(x->buffer,"input"))
                {
                    portflags = portflags | JackPortIsInput;
                    keyflag = 1;
                }

                if (!strcmp(x->buffer,"output"))
                {
                    portflags = portflags | JackPortIsOutput;
                    keyflag = 1;
                }
                if (!keyflag && !expflag)
                {
                    strcpy(x->expression, x->buffer);
                    expflag = 1;
                }


            }


        }


        ports = jack_get_ports (jc, x->expression,NULL,portflags|JackPortIsOutput);
        n=0;
        if (ports)
        {
            while (ports[n])
            {
                //seperate port and client

                l = strlen(ports[n]);
                t = strchr(ports[n],':');

                if (t)
                {
                    s_port=gensym(strchr(ports[n],':')+1);


                    snprintf(x->buffer,l-strlen(s_port->s_name),ports[n]);
                    s_client = gensym(x->buffer);

                    SETSYMBOL(x->a_outlist,s_client);
                    SETSYMBOL(x->a_outlist+1,s_port);

                    // output in output-outlet
                    outlet_list(x->output_ports,&s_list,2, x->a_outlist);
                }

                n++;
            }
        }
        free(ports);

        ports = jack_get_ports (jc, x->expression,NULL,portflags|JackPortIsInput);
        n=0;
        if (ports)
        {
            while (ports[n])
            {
                l = strlen(ports[n]);
                t = strchr(ports[n],':');

                if (t)
                {
                    s_port=gensym(strchr(ports[n],':')+1);


                    snprintf(x->buffer,l-strlen(s_port->s_name),ports[n]);
                    s_client = gensym(x->buffer);

                    SETSYMBOL(x->a_outlist,s_client);
                    SETSYMBOL(x->a_outlist+1,s_port);

                    // output in output-outlet
                    outlet_list(x->input_ports,&s_list,2, x->a_outlist);
                }


                n++;
            }
        }
        free(ports);

        strcpy(x->expression,"");//reset regex
    }

}

static void *jackports_new(void)
{
    t_jackports * x = (t_jackports *)pd_new(jackports_class);
    x->output_ports = outlet_new(&x->x_obj, &s_list);
    x->input_ports = outlet_new(&x->x_obj, &s_list);
    x->a_outlist = getbytes(3 * sizeof(t_atom));
    x->buffer = getbytes(128);

    return (void*)x;
}

static void setup(void)
{
    jc = jackx_get_jack_client();

    jackports_class = class_new(gensym("jack-ports"),
                                (t_newmethod)jackports_new,
                                0,
                                sizeof(t_jackports),
                                CLASS_DEFAULT,
                                0);
    class_addanything(jackports_class, jackports_input);
}

void setup_jack0x2dports(void)
{
    setup();
}
