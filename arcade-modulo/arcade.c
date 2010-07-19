/*  SPANISH ________________________________________________________
 *  arcade.c - Driver usando el puerto paralelo para Maquinas Arcade
 *  Por David Colmenero AKA D_Skywalk <dantoine@gmail.com>
 *  
 *  Basado en gamecon.c
 *   (autores ver abajo)
 *
 *  ENGLISH _________________________________________________________
 *  
 *  arcade.c - Driver for Arcade Cabinets using paralel port
 *  By David Colmenero AKA D_Skywalk <dantoine@gmail.com>
 *  
 *  Based on gamecon.c 
 *  GAMECON credits:
 *
 *  Copyright (c) 1999-2004	Vojtech Pavlik <vojtech@suse.cz>
 *  Copyright (c) 2004		Peter Nelson <rufus-kernel@hackish.org>
 *
 *  Based on the work of:
 *	Andree Borrmann		John Dahlstrom
 *	David Kuder		Nathan Hand
 *
 *
 *
 *  OpenPPJoy - Copyright (c) 2006
 *		David Colmenero Jimenez
 *  Version 0.4a
 */
 
/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <dskywalk@gmail.com>
 */
 
/*
 * Changelog - OpenPPJoy
 *
 * 0.4a - add 2.6.15 kernel required changes & more extra keys.
 * 0.4  - First public version.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/parport.h>
#include <linux/input.h>


//UNCOMMENT FOR DEBUG :D
//#define DEBUG_SPP

MODULE_AUTHOR("David Colmenero aka [D_Skywalk | Dantoine] <dskywalk@gmail.org>");
MODULE_DESCRIPTION("A LPT Arcade Cabinet Driver for Linux");
MODULE_LICENSE("GPL");

#define GC_MAX_PORTS		1
#define GC_MAX_DEVICES		5

struct gc_config {
	int args[GC_MAX_DEVICES + 1];
	int nargs;
};

static struct gc_config gc[GC_MAX_PORTS] __initdata;

module_param_array_named(map, gc[0].args, int, &gc[0].nargs, 0);
MODULE_PARM_DESC(map, "Describes first set of devices (<parport#>,<pad1>,<pad2>,..<pad5>)");

#if GC_MAX_PORTS > 3

	module_param_array_named(map2, gc[1].args, int, &gc[1].nargs, 0);
	MODULE_PARM_DESC(map2, "Describes second set of devices");
	module_param_array_named(map3, gc[2].args, int, &gc[2].nargs, 0);
	MODULE_PARM_DESC(map3, "Describes third set of devices");

#endif


////////////////////////////////////
//INITIAL PARAMETERS AND STRUCTS

//control types
#define GC_ARCADE 1
#define GC_KEY1 2
#define GC_KEY2 3
#define GC_KEY3 4
#define GC_MAX 4

#define GC_REFRESH_TIME HZ/100

struct gc {
	struct pardevice *pd;
	struct input_dev *dev[GC_MAX_DEVICES];
	struct timer_list timer;
	unsigned char pads[GC_MAX +1];
	int used;
	struct semaphore sem;
	char phys[GC_MAX_DEVICES][32];
};

static struct gc *gc_base[GC_MAX_PORTS];

static int gc_status_bit[]={ 0x40, 0x80, 0x20, 0x10, 0x08 };
static char * gc_names[]={ NULL, "Arcade Cabinet Control",  "Arcade Cabinet Control Key1", "Arcade Cabinet Control Key2", "Arcade Cabinet Control Key3" };

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
	
	
	udelay(GC_ARCADE_DELAY); //waiting
	
	for (i = 0; i < ( tam - 4 ); i++) { //write 8 bits
		parport_write_data(gc->pd->port, ~(1 << i)); //11111011 (0 open)
		data[i] = parport_read_status(gc->pd->port) ^ 0x7f;
		
		#ifdef DEBUG_SPP
		if( (0x20 & data[i]) != 0)
			printk(KERN_INFO "arcade.c: Read in %i! DATA[%x] \n",i,(0x20 & data[i]));
		#endif
		
	}
	
	parport_write_data(gc->pd->port, 0xff); //cleaning
	for (i = 8; (i < tam); i++) {
		
		if( i != 10 ) //INIT LINE
			parport_write_control(gc->pd->port, (1 << (i - 8) | PARPORT_CONTROL_INIT));
		else
			parport_write_control(gc->pd->port, 0x0); //Activing INIT line (works invert)
		
		data[i] = parport_read_status(gc->pd->port) ^ 0x7f; //filtring
		
		#ifdef DEBUG_SPP
		tmp = parport_read_control(gc->pd->port);
		if(test[(i - 8)] != data[i]){ //debug
			test[(i - 8)] = data[i];
			printk(KERN_INFO "arcade.c: HI - Change in %i! DATA[%x] - CONTROL[%x]\n",i, data[i], tmp);	
		}
		#endif
	}
	
	parport_write_control(gc->pd->port, PARPORT_CONTROL_INIT); //clean

}



#define GC_MAX_LENGTH GC_ARCADE_LENGTH

static void arcade_control_process_packet(struct gc *gc)
{

	unsigned char data[GC_MAX_LENGTH];
	struct input_dev *dev;
	int i, s;

	memset(data, 0, sizeof(char) * GC_MAX_LENGTH); //clean
	gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

	for (i = 0; i < GC_MAX_DEVICES; i++) {

		dev = gc->dev[i];
		if (!dev)
			continue;

		s = gc_status_bit[i]; //preparamos el bit a mirar

		if (s & (gc->pads[GC_ARCADE] )) { //comprueba si el pad esta cargado
			input_report_abs(dev, ABS_X,  !(s & data[2]) - !(s & data[3])); //der o izq
			input_report_abs(dev, ABS_Y,  !(s & data[0]) - !(s & data[1])); // arriba o abajo
			input_report_key(dev, BTN_0, s & data[4]); //Pulsador 1
			input_report_key(dev, BTN_1, s & data[5]); //Pulsador 2
			input_report_key(dev, BTN_2, s & data[6]); //Pulsador 3
			input_report_key(dev, BTN_3, s & data[7]); //Pulsador 4
			//___________ USING CONTROL LINE ___________
			input_report_key(dev, BTN_4, s & data[8]); //Pulsador 5
			input_report_key(dev, BTN_5, s & data[9]); //Pulsador 6
			input_report_key(dev, BTN_6, s & data[10]); //Pulsador 7
			input_report_key(dev, BTN_7, s & data[11]); //Pulsador 8

		}

		input_sync(dev);
	}

}


static void arcade_keyboard1_process_packet(struct gc *gc)
{

	unsigned char data[GC_MAX_LENGTH];
	struct input_dev *dev;
	int i, s;

	memset(data, 0, sizeof(char) * GC_MAX_LENGTH); //clean
	gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

	for (i = 0; i < GC_MAX_DEVICES; i++) {

		dev = gc->dev[i];
		if (!dev)
			continue;

		s = gc_status_bit[i]; // preparamos el bit a mirar

		if (s & (gc->pads[GC_KEY1] )) { //comprueba si el pad esta cargado
			input_report_key(dev, KEY_UP, s & data[0]); //arriba
			input_report_key(dev, KEY_DOWN, s & data[1]); //abajo
			input_report_key(dev, KEY_LEFT, s & data[2]); //izq
			input_report_key(dev, KEY_RIGHT, s & data[3]); //der
			input_report_key(dev, KEY_LEFTCTRL, s & data[4]); //disparo 1
			input_report_key(dev, KEY_LEFTSHIFT, s & data[5]); //disparo 2
			input_report_key(dev, KEY_LEFTALT, s & data[6]); //disparo 3
			input_report_key(dev, KEY_T, s & data[7]); //disparo 4
			//___________ USING CONTROL LINE ___________
			input_report_key(dev, KEY_Y, s & data[8]); //disparo 5
			input_report_key(dev, KEY_U, s & data[9]); //disparo 6
			input_report_key(dev, KEY_5, s & data[10]); //MONEDA 1
			input_report_key(dev, KEY_1, s & data[11]);  //1P
		}

		input_sync(dev);
	}
}


static void arcade_keyboard2_process_packet(struct gc *gc)
{

	unsigned char data[GC_MAX_LENGTH];
	struct input_dev *dev;
	int i, s;

	memset(data, 0, sizeof(char) * GC_MAX_LENGTH); //clean
	gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

	for (i = 0; i < GC_MAX_DEVICES; i++) {

		dev = gc->dev[i];
		if (!dev)
			continue;

		s = gc_status_bit[i]; // preparamos el bit a mirar

		if (s & (gc->pads[GC_KEY2] )) { //comprueba si el pad esta cargado
			input_report_key(dev, KEY_W, s & data[0]); //arriba
			input_report_key(dev, KEY_S, s & data[1]); //abajo
			input_report_key(dev, KEY_A, s & data[2]); //izq
			input_report_key(dev, KEY_D, s & data[3]); //der
			input_report_key(dev, KEY_Q, s & data[4]); //disparo 1
			input_report_key(dev, KEY_E, s & data[5]); //disparo 2
			input_report_key(dev, KEY_Z, s & data[6]); //disparo 3
			input_report_key(dev, KEY_X, s & data[7]); //disparo 4
			//___________ USING CONTROL LINE ___________
			input_report_key(dev, KEY_C, s & data[8]); //disparo 5
			input_report_key(dev, KEY_V, s & data[9]); //disparo 6
			input_report_key(dev, KEY_6, s & data[10]); //SELECCION
			input_report_key(dev, KEY_2, s & data[11]);  //COMENZAR

		}

		input_sync(dev);
	}

}


static void arcade_keyboard3_process_packet(struct gc *gc)
{

	unsigned char data[GC_MAX_LENGTH];
	struct input_dev *dev;
	int i, s;

	memset(data, 0, sizeof(char) * GC_MAX_LENGTH); //clean
	gc_arcade_read_packet(gc, GC_ARCADE_LENGTH, data);

	for (i = 0; i < GC_MAX_DEVICES; i++) {

		dev = gc->dev[i];
		if (!dev)
			continue;

		s = gc_status_bit[i]; // preparamos el bit a mirar

		if (s & (gc->pads[GC_KEY2] )) { //comprueba si el pad esta cargado
			input_report_key(dev, KEY_N, s & data[0]); //extra 1
			input_report_key(dev, KEY_M, s & data[1]); //extra 2
			input_report_key(dev, KEY_P, s & data[2]); //pause
			input_report_key(dev, KEY_ENTER, s & data[3]); 
			input_report_key(dev, KEY_ESC, s & data[4]); 
			input_report_key(dev, KEY_TAB, s & data[5]); 
			input_report_key(dev, KEY_SPACE, s & data[6]); 
			input_report_key(dev, KEY_GRAVE, s & data[7]); //º
			//___________ USING CONTROL LINE ___________
			input_report_key(dev, KEY_3, s & data[8]); //3P
			input_report_key(dev, KEY_4, s & data[9]); //4P
			input_report_key(dev, KEY_7, s & data[10]); //moneda 3
			input_report_key(dev, KEY_8, s & data[11]);  //moneda 4

		}

		input_sync(dev);
	}

}


static void arcade_timer(unsigned long private)
{
	struct gc *gc = (void *) private;

	
/*
 * Arcade joysticks
 */

	if (gc->pads[GC_ARCADE] ) 
		arcade_control_process_packet(gc);


/*
 * Keyboards Configs
 */

	if (gc->pads[GC_KEY1] ) 
		arcade_keyboard1_process_packet(gc);

	if (gc->pads[GC_KEY2] ) 
		arcade_keyboard2_process_packet(gc);
		
	if (gc->pads[GC_KEY3] ) 
		arcade_keyboard3_process_packet(gc);
	

	mod_timer(&gc->timer, jiffies + GC_REFRESH_TIME);

}

static int arcade_open(struct input_dev *dev)
{
	struct gc *gc = input_get_drvdata(dev);
	int err;

	err = down_interruptible(&gc->sem);
	
	if (err)
		return err;

	if (!gc->used++) {
		parport_claim(gc->pd);
		parport_write_control(gc->pd->port, 0x04);
		mod_timer(&gc->timer, jiffies + GC_REFRESH_TIME);
	}

	up(&gc->sem);
	return 0;
}

static void arcade_close(struct input_dev *dev)
{
	struct gc *gc = input_get_drvdata(dev);

	down(&gc->sem);
	if (!--gc->used) {
		del_timer(&gc->timer);
		parport_write_control(gc->pd->port, 0x00);
		parport_release(gc->pd);
	}

	up(&gc->sem);
}


static int __init arcade_setup_control(struct gc *gc, int idx, int pad_type)
{
	struct input_dev *input_dev;
	int i;

	if (!pad_type)
		return 0;

	if (pad_type < 1 || pad_type > GC_MAX) {
		printk(KERN_WARNING "arcade.c: Pad type %d unknown\n", pad_type);
		return -EINVAL;
	}


	gc->dev[idx] = input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "arcade.c: Not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = gc_names[pad_type];
	input_dev->phys = gc->phys[idx];
	input_dev->id.bustype = BUS_PARPORT;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = pad_type;
	input_dev->id.version = 0x0100;

	input_set_drvdata(input_dev, gc);

	input_dev->open = arcade_open;
	input_dev->close = arcade_close;
	
	input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);

	if(pad_type == GC_ARCADE){
		for (i = 0; i < 4; i++)
			input_set_abs_params(input_dev, ABS_X + i, -1, 1, 0, 0);
	}

	
	gc->pads[0] |= gc_status_bit[idx];
	gc->pads[pad_type] |= gc_status_bit[idx];

	switch(pad_type) {
		case GC_ARCADE:
			set_bit(BTN_0, input_dev->keybit); //configurando mando...
			set_bit(BTN_1, input_dev->keybit);
			set_bit(BTN_2, input_dev->keybit);
			set_bit(BTN_3, input_dev->keybit);
			set_bit(BTN_4, input_dev->keybit);
			set_bit(BTN_5, input_dev->keybit);
			set_bit(BTN_6, input_dev->keybit);
			set_bit(BTN_7, input_dev->keybit);
			break;

		case GC_KEY1:
			set_bit(KEY_UP, 	input_dev->keybit); //configurando teclado...
			set_bit(KEY_DOWN, 	input_dev->keybit);
			set_bit(KEY_LEFT, 	input_dev->keybit);
			set_bit(KEY_RIGHT, 	input_dev->keybit);
			set_bit(KEY_LEFTCTRL, 	input_dev->keybit);
			set_bit(KEY_LEFTSHIFT, 	input_dev->keybit);
			set_bit(KEY_LEFTALT, 	input_dev->keybit);
			set_bit(KEY_T, 		input_dev->keybit);
			set_bit(KEY_Y, 		input_dev->keybit);
			set_bit(KEY_U, 		input_dev->keybit);
			set_bit(KEY_5, 		input_dev->keybit);
			set_bit(KEY_1, 		input_dev->keybit);
			break;

		case GC_KEY2:
			set_bit(KEY_W, input_dev->keybit); //configurando mas teclado...
			set_bit(KEY_S, input_dev->keybit);
			set_bit(KEY_A, input_dev->keybit);
			set_bit(KEY_D, input_dev->keybit);
			set_bit(KEY_Q, input_dev->keybit);
			set_bit(KEY_E, input_dev->keybit);
			set_bit(KEY_Z, input_dev->keybit);
			set_bit(KEY_X, input_dev->keybit);
			set_bit(KEY_C, input_dev->keybit);
			set_bit(KEY_V, input_dev->keybit);
			set_bit(KEY_6, input_dev->keybit);
			set_bit(KEY_2, input_dev->keybit);
			break;
			
		case GC_KEY3:
			set_bit(KEY_N, input_dev->keybit); //configurando mas teclado...
			set_bit(KEY_M, input_dev->keybit);
			set_bit(KEY_P, input_dev->keybit);
			set_bit(KEY_ENTER, input_dev->keybit);
			set_bit(KEY_ESC, input_dev->keybit);
			set_bit(KEY_TAB, input_dev->keybit);
			set_bit(KEY_SPACE, input_dev->keybit);
			set_bit(KEY_GRAVE, input_dev->keybit); //buscando º
			set_bit(KEY_3, input_dev->keybit);
			set_bit(KEY_4, input_dev->keybit);
			set_bit(KEY_7, input_dev->keybit);
			set_bit(KEY_8, input_dev->keybit);
	}
	return 0;
}


static struct gc __init *arcade_probe(int parport, int *pads, int n_pads)
{

	struct gc *gc;
	struct parport *pp;
	struct pardevice *pd;
	int i;
	int err;

	pp = parport_find_number(parport);
	if (!pp) {
		printk(KERN_ERR "arcade.c: no such parport\n");
		err = -EINVAL;
		goto err_out;
	}

	pd = parport_register_device(pp, "arcade", NULL, NULL, NULL, PARPORT_DEV_EXCL, NULL);
	if (!pd) {
		printk(KERN_ERR "arcade.c: parport busy already - lp.o loaded?\n");
		err = -EBUSY;
		goto err_put_pp;
	}

	gc = kzalloc(sizeof(struct gc), GFP_KERNEL);
	if (!gc) {
		printk(KERN_ERR "arcade.c: Not enough memory\n");
		err = -ENOMEM;
		goto err_unreg_pardev;
	}

	init_MUTEX(&gc->sem);
	gc->pd = pd;
	init_timer(&gc->timer);
	gc->timer.data = (long) gc;
	gc->timer.function = arcade_timer;
	
	for (i = 0; i < n_pads && i < GC_MAX_DEVICES; i++) {
		if (!pads[i])
			continue;

		sprintf(gc->phys[i], "%s/control%d", gc->pd->port->name, i);
		err = arcade_setup_control(gc, i, pads[i]);
		if (err)
			goto err_unreg_devs;

		err = input_register_device(gc->dev[i]);
		if (err)
			goto err_free_dev;
	}

	if (!gc->pads[0]) {
		printk(KERN_ERR "arcade.c: No valid devices specified\n");
		err = -EINVAL;
		goto err_free_gc;
	}

	parport_put_port(pp);
	return gc;

 err_free_dev:
	input_free_device(gc->dev[i]);
 err_unreg_devs:
	while (--i >= 0)
		if (gc->dev[i])
			input_unregister_device(gc->dev[i]);
 err_free_gc:
	kfree(gc);
 err_unreg_pardev:
	parport_unregister_device(pd);
 err_put_pp:
	parport_put_port(pp);
 err_out:
	return ERR_PTR(err);
}

static void arcade_remove(struct gc *gc)
{
	int i;

	for (i = 0; i < GC_MAX_DEVICES; i++)
		if (gc->dev[i])
			input_unregister_device(gc->dev[i]);
	parport_unregister_device(gc->pd);
	kfree(gc);
}


static int __init arcade_init(void)
{
	int i;
	int have_dev = 0;
	int err = 0;

	printk(KERN_INFO "arcade.c: Initializing Arcade module - v0.4a\n");	

	for (i = 0; i < GC_MAX_PORTS; i++) {
		if (gc[i].nargs == 0 || gc[i].args[0] < 0)
			continue;

		if (gc[i].nargs < 2) {
			printk(KERN_ERR "arcade.c: at least one device must be specified\n");
			err = -EINVAL;
			break;
		}

		gc_base[i] = arcade_probe(gc[i].args[0], gc[i].args + 1, gc[i].nargs - 1);
		if (IS_ERR(gc_base[i])) {
			err = PTR_ERR(gc_base[i]);
			break;
		}

		have_dev = 1;
	}

	if (err) {
		while (--i >= 0)
			if (gc_base[i])
				arcade_remove(gc_base[i]);
		return err;
	}

	return have_dev ? 0 : -ENODEV;
}



static void __exit arcade_exit(void)
{
	int i;

	printk(KERN_INFO "arcade.c: Freeying Arcade module.\n");	

	for (i = 0; i < GC_MAX_PORTS; i++)
		if (gc_base[i])
			arcade_remove(gc_base[i]);

	printk(KERN_INFO "arcade.c: Arcade Module finished correctly.\n");

}



module_init(arcade_init);
module_exit(arcade_exit);
