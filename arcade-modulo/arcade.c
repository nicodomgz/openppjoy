/*
 *  arcade.c - Driver usando el puerto paralelo para Maquinas Arcade
 *  Por David Colmenero AKA D_Skywalk <dantoine@gmail.com>
 *
 *  Basado en gamecon.c de:
 *               Vojtech Pavlik <vojtech@suse.cz>
 *               Peter Nelson <rufus-kernel@hackish.org>
 *  Version 0.4
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>		/* para macros */
#include <linux/parport.h>
#include <linux/input.h>

#include "arcade.h"


//DESCOMENTA ESTO SI QUIERES DEBUGEAR :D
//#define DEBUG_SPP

#define DRIVER_AUTHOR "David Colmenero aka [D_Skywalk | Dantoine] <dskywalk@gmail.org>"
#define DRIVER_DESC   "A LPT Arcade Cabinet Driver for Linux"


static int gc[] __initdata = {-1,0,0,0,0,0};
static int gc_nargs __initdata = 0;
module_param_array_named(map, gc, int, &gc_nargs,0);
MODULE_PARM_DESC(map, "device (<parport#>, <pad1>, <pad2>, ...<pad5>)");

__obsolete_setup("gc=");

////////////////////////////////////
//PARAMETROS INICIALES Y ESTRUCTURAS

//tipos de controles
#define GC_ARCADE 1
#define GC_KEY1 2
#define GC_KEY2 3
#define GC_MAX 3

#define GC_REFRESH_TIME HZ/100

struct gc {
	struct pardevice *pd;
	struct input_dev dev[5];
	struct timer_list timer;
	unsigned char pads[GC_MAX +1];
	int used;
	char phys[5][32];
};

static struct gc * gc_base; //Aqui hay que meter mas si vamos a dar soporte a mas lpts, maximo [3]
static int gc_status_bit[]={ 0x40, 0x80, 0x20, 0x10, 0x08 };
static char * gc_names[]={ NULL, "Arcade Cabinet Control",  "Arcade Cabinet Control Key1", "Arcade Cabinet Control Key2" };

/*
* Driver Arcade
*/

#define GC_ARCADE_LENGTH 12
#define GC_ARCADE_DELAY 15


static void gc_arcade_read_packet(struct gc *gc, int tam, unsigned char *data) 
{
	unsigned char i;
	
	#ifdef DEBUG_SPP
	unsigned char tmp;
	static int test[4]={0xf,0xf,0xf,0xf};
	#endif
	
	
	udelay(GC_ARCADE_DELAY); //esperamos	
	
	for (i = 0; i < ( tam - 4 ); i++) { //escribimos 8 bits
		parport_write_data(gc->pd->port, ~(1 << i)); //11111011 (0 abierto)
		data[i] = parport_read_status(gc->pd->port) ^ 0x7f;
		
		#ifdef DEBUG_SPP
		if( (0x20 & data[i]) != 0)
			printk(KERN_INFO "arcade.c: Leido en %i! DATA[%x] \n",i,(0x20 & data[i]));
		#endif
		
	}
	
	parport_write_data(gc->pd->port, 0xff); //limpiamos
	for (i = 8; (i < tam); i++) {
		
		if( i != 10 ) //LINEA INIT
			parport_write_control(gc->pd->port, (1 << (i - 8) | PARPORT_CONTROL_INIT));
		else
			parport_write_control(gc->pd->port, 0x0); //Activamos la linea INIT (que esta invertida)
		
		data[i] = parport_read_status(gc->pd->port) ^ 0x7f; //filtramos
		
		#ifdef DEBUG_SPP
		tmp = parport_read_control(gc->pd->port);
		if(test[(i - 8)] != data[i]){ //debug
			test[(i - 8)] = data[i];
			printk(KERN_INFO "arcade.c: HI - Cambio en %i! DATA[%x] - CONTROL[%x]\n",i, data[i], tmp);	
		}
		#endif
	}
	
	parport_write_control(gc->pd->port, PARPORT_CONTROL_INIT); //limpiamos

}

#define GC_MAX_LENGTH GC_ARCADE_LENGTH


static void arcade_timer(unsigned long private)
{
	struct gc *gc = (void *) private;
	struct input_dev *dev = gc->dev;
	unsigned char data[GC_MAX_LENGTH];
	int s;
	int i;

	memset(data, 0, sizeof(char) * GC_MAX_LENGTH); //limpiando
	
/*
 * Arcade joysticks
 */
	gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

	if (gc->pads[GC_ARCADE] ) {


		for (i = 0; i < 5; i++) {

			s = gc_status_bit[i]; // preparamos el bit a mirar

			if (s & (gc->pads[GC_ARCADE] )) { //comprueba si el pad esta cargado
				input_report_abs(dev + i, ABS_X,  !(s & data[2]) - !(s & data[3])); //der o izq
				input_report_abs(dev + i, ABS_Y,  !(s & data[0]) - !(s & data[1])); // arriba o abajo
				input_report_key(dev + i, BTN_0, s & data[4]); //Pulsador 1
				input_report_key(dev + i, BTN_1, s & data[5]); //Pulsador 2
				input_report_key(dev + i, BTN_2, s & data[6]); //Pulsador 3
				input_report_key(dev + i, BTN_3, s & data[7]); //Pulsador 4
				//___________ A PARTIR DE AQUI USAMOS LA LINEA DE CONTROL ___________
				input_report_key(dev + i, BTN_4, s & data[8]); //Pulsador 5
				input_report_key(dev + i, BTN_5, s & data[9]); //Pulsador 6
				input_report_key(dev + i, BTN_6, s & data[10]); //Pulsador 7
				input_report_key(dev + i, BTN_7, s & data[11]); //Pulsador 8

			}

			input_sync(dev + i);
		}


	}


	if (gc->pads[GC_KEY1] ) {

		//gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

		for (i = 0; i < 5; i++) {

			s = gc_status_bit[i]; // preparamos el bit a mirar

			if (s & (gc->pads[GC_KEY1] )) { //comprueba si el pad esta cargado
				input_report_key(dev + i, KEY_UP, s & data[0]); //arriba
				input_report_key(dev + i, KEY_DOWN, s & data[1]); //abajo
				input_report_key(dev + i, KEY_LEFT, s & data[2]); //izq
				input_report_key(dev + i, KEY_RIGHT, s & data[3]); //der
				input_report_key(dev + i, KEY_SPACE, s & data[4]); //disparo 1
				input_report_key(dev + i, KEY_LEFTCTRL, s & data[5]); //disparo 2
				input_report_key(dev + i, KEY_LEFTSHIFT, s & data[6]); //disparo 3
				input_report_key(dev + i, KEY_LEFTALT, s & data[7]); //disparo 4
				//___________ A PARTIR DE AQUI USAMOS LA LINEA DE CONTROL ___________
				input_report_key(dev + i, KEY_ESC, s & data[8]); //salir
				input_report_key(dev + i, KEY_ENTER, s & data[9]); //entrar
				input_report_key(dev + i, KEY_5, s & data[10]); //MONEDA
				input_report_key(dev + i, KEY_1, s & data[11]);  //1P

			}

			input_sync(dev + i);
		}


	}

	if (gc->pads[GC_KEY2] ) {

		//gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

		for (i = 0; i < 5; i++) {

			s = gc_status_bit[i]; // preparamos el bit a mirar

			if (s & (gc->pads[GC_KEY2] )) { //comprueba si el pad esta cargado
				input_report_key(dev + i, KEY_W, s & data[0]); //arriba
				input_report_key(dev + i, KEY_S, s & data[1]); //abajo
				input_report_key(dev + i, KEY_A, s & data[2]); //izq
				input_report_key(dev + i, KEY_D, s & data[3]); //der
				input_report_key(dev + i, KEY_Q, s & data[4]); //disparo 1
				input_report_key(dev + i, KEY_E, s & data[5]); //disparo 2
				input_report_key(dev + i, KEY_Z, s & data[6]); //disparo 3
				input_report_key(dev + i, KEY_X, s & data[7]); //disparo 4
				//___________ A PARTIR DE AQUI USAMOS LA LINEA DE CONTROL ___________
				input_report_key(dev + i, KEY_C, s & data[8]); //disparo 5
				input_report_key(dev + i, KEY_V, s & data[9]); //disparo 6
				input_report_key(dev + i, KEY_6, s & data[10]); //SELECCION
				input_report_key(dev + i, KEY_2, s & data[11]);  //COMENZAR

			}

			input_sync(dev + i);
		}


	}

	mod_timer(&gc->timer, jiffies + GC_REFRESH_TIME);

}

static int arcade_open(struct input_dev *dev)
{
	struct gc *gc = dev->private;
	if (!gc->used++) {
		parport_claim(gc->pd);
		parport_write_control(gc->pd->port, 0x04);
		mod_timer(&gc->timer, jiffies + GC_REFRESH_TIME);
	}
	return 0;
}

static void arcade_close(struct input_dev *dev)
{
	struct gc *gc = dev->private;
	if (!--gc->used) {
		del_timer(&gc->timer);
		parport_write_control(gc->pd->port, 0x00);
		parport_release(gc->pd);
	}
}


static struct gc __init *arcade_probe(int *config, int nargs)
{
	struct gc *gc;
	struct parport *pp;
	int i, j;

	if (config[0] < 0)
		return NULL;

	if (nargs < 2) {
		printk(KERN_ERR "arcade.c: al menos un dispositivo debe ser configurado\n");
		return NULL;
	}
	
	pp = parport_find_number(config[0]);

	if (!pp) {
		printk(KERN_ERR "arcade.c: no encontre puerto paralelo\n");
		return NULL;
	}

	if (!(gc = kmalloc(sizeof(struct gc), GFP_KERNEL))) {
		parport_put_port(pp);
		return NULL;
	}
	memset(gc, 0, sizeof(struct gc));

	gc->pd = parport_register_device(pp, "arcade", NULL, NULL, NULL, PARPORT_DEV_EXCL, NULL);

	parport_put_port(pp);

	if (!gc->pd) {
		printk(KERN_ERR "arcade.c: puerto ya ocupado - lp.o cargado?\n");
		kfree(gc);
		return NULL;
	}

	parport_claim(gc->pd);


	init_timer(&gc->timer);
	gc->timer.data = (long) gc;
	gc->timer.function = arcade_timer;

	//nargs son los joys que se cargan
	for (i = 0; i < nargs - 1; i++) {

		if (!config[i + 1])
			continue;


		if (config[i + 1] < 1 || config[i + 1] > GC_MAX) {
			printk(KERN_WARNING "arcade.c: tipo de control %d desconocido\n", config[i + 1]);
			continue;
		}
		
		//TODO: COMPROBAR QUE NO SE CARGA EL TECLADO(1/2) MAS DE UNA VEZ :?

        gc->dev[i].private = gc;
        gc->dev[i].open = arcade_open;
        gc->dev[i].close = arcade_close;

        gc->dev[i].evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
		

		//no configurar ejes en teclado...
		if(config[i + 1] == GC_ARCADE){
			for (j = 0; j < 2; j++) {
				set_bit(ABS_X + j, gc->dev[i].absbit);
				gc->dev[i].absmin[ABS_X + j] = -1;
				gc->dev[i].absmax[ABS_X + j] =  1;
			}
		}
		
		
		gc->pads[0] |= gc_status_bit[i];
		gc->pads[config[i + 1]] |= gc_status_bit[i];
		

		switch(config[i + 1]) {

			case GC_ARCADE:
				set_bit(BTN_0, gc->dev[i].keybit); //configurando mando...
				set_bit(BTN_1, gc->dev[i].keybit);
				set_bit(BTN_2, gc->dev[i].keybit);
				set_bit(BTN_3, gc->dev[i].keybit);
				set_bit(BTN_4, gc->dev[i].keybit);
				set_bit(BTN_5, gc->dev[i].keybit);
				set_bit(BTN_6, gc->dev[i].keybit);
				set_bit(BTN_7, gc->dev[i].keybit);
				break;

	
			case GC_KEY1:
				set_bit(KEY_UP, gc->dev[i].keybit); //configurando teclado...
				set_bit(KEY_DOWN, gc->dev[i].keybit);
				set_bit(KEY_LEFT, gc->dev[i].keybit);
				set_bit(KEY_RIGHT, gc->dev[i].keybit);
				set_bit(KEY_SPACE, gc->dev[i].keybit);
				set_bit(KEY_LEFTCTRL, gc->dev[i].keybit);
				set_bit(KEY_LEFTSHIFT, gc->dev[i].keybit);
				set_bit(KEY_LEFTALT, gc->dev[i].keybit);
				set_bit(KEY_ESC, gc->dev[i].keybit);
				set_bit(KEY_ENTER, gc->dev[i].keybit);
				set_bit(KEY_5, gc->dev[i].keybit);
				set_bit(KEY_1, gc->dev[i].keybit);
				break;

			case GC_KEY2:
				set_bit(KEY_W, gc->dev[i].keybit); //configurando mas teclado...
				set_bit(KEY_S, gc->dev[i].keybit);
				set_bit(KEY_A, gc->dev[i].keybit);
				set_bit(KEY_D, gc->dev[i].keybit);
				set_bit(KEY_Q, gc->dev[i].keybit);
				set_bit(KEY_E, gc->dev[i].keybit);
				set_bit(KEY_Z, gc->dev[i].keybit);
				set_bit(KEY_X, gc->dev[i].keybit);
				set_bit(KEY_C, gc->dev[i].keybit);
				set_bit(KEY_V, gc->dev[i].keybit);
				set_bit(KEY_6, gc->dev[i].keybit);
				set_bit(KEY_2, gc->dev[i].keybit);
				break;
		}

		sprintf(gc->phys[i], "%s/control%d", gc->pd->port->name, i);

                gc->dev[i].name = gc_names[config[i + 1]];
		gc->dev[i].phys = gc->phys[i];
                gc->dev[i].id.bustype = BUS_PARPORT;
                gc->dev[i].id.vendor = 0x0001;
                gc->dev[i].id.product = config[i + 1];
                gc->dev[i].id.version = 0x0100;
	
	}

	parport_release(gc->pd);

	if (!gc->pads[0]) {
		parport_unregister_device(gc->pd);
		kfree(gc);
		return NULL;
	}

	for (i = 0; i < 5; i++)
		if (gc->pads[0] & gc_status_bit[i]) {
			input_register_device(gc->dev + i);
			printk(KERN_INFO "control: %s en %s\n", gc->dev[i].name, gc->pd->port->name);
		}
		
	printk(KERN_INFO "arcade.c: Modulo Arcade Cargado Correctamente!\n");	
		
	return gc;
}


static int __init arcade_init(void) 
{

	printk(KERN_INFO "arcade.c: Iniciando modulo Arcade - v0.4\n");	

	gc_base = arcade_probe(gc, gc_nargs);

	if (gc_base) //|| gc_base[1] || gc_base[2])
		return 0;

	return -ENODEV;

        //return 0; //sino se retorna 0 significa que no se cargo...
}

static void __exit arcade_exit(void) 
{

	int j; //i

	printk(KERN_INFO "arcade.c: Descargando modulo Arcade.\n");	


	//for (i = 0; i < 3; i++)
		if (gc_base){ //[i]) {
			for (j = 0; j < 5; j++)
				if (gc_base->pads[0] & gc_status_bit[j]) //[i]
					input_unregister_device(gc_base->dev + j); //[i]
			parport_unregister_device(gc_base->pd); //[i]
		}
		
	printk(KERN_INFO "arcade.c: Modulo Arcade finalizado correctamente.\n");
}



module_init(arcade_init);
module_exit(arcade_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
