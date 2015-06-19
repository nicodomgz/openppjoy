# How made a better driver and implementation #

At the moment the driver is not configurable, but it is full working if you create a panel for him... this panel would be compatible with PPjoy closed code version (see configuration modes in his documentation).

There are some info that you need read:
  * Marcianitos LPT tutorial (spanish)
[http://www.marcianitos.org/tutoriales/IC018.htm (archive.org)](http://web.archive.org/web/20070203140435/http://www.marcianitos.org/tutoriales/IC018.htm)
  * The discussion topic about OpenPPJoy (Spanish too):
http://www.retrovicio.com/foro/showthread.php?t=5826

_(English versions are welcomed)<_

With PPJoy a good configuration will be:

```
[           ][    10    ][    11     ][    12    ][    13    ][    15    ]
[     2     ][  1-UP    ][  1-DOWN   ][  1-LEFT  ][  1-RIGHT ][  1-FIRE1 ]
[     3     ][  1-FIRE2 ][  1-FIRE3  ][  1-FIRE4 ][  1-FIRE5 ][  1-FIRE6 ]
[     4     ][          ][           ][          ][          ][          ]
[     5     ][          ][           ][          ][          ][          ]
[     6     ][          ][           ][          ][          ][          ]
[     7     ][          ][           ][          ][          ][          ]
[     8     ][  2-UP    ][  2-DOWN   ][  2-LEFT  ][  2-RIGHT ][  2-FIRE1 ]
[     9     ][  2-FIRE2 ][  2-FIRE3  ][  2-FIRE4 ][  2-FIRE5 ][  2-FIRE6 ]
[     1     ][          ][           ][          ][          ][          ]
[    14     ][          ][           ][          ][          ][          ]
[    16     ][          ][           ][          ][          ][          ]
[    17     ][          ][           ][          ][          ][          ]
```

But with OpenPPJoy for to make a simple implementation I resolved to configure using columns:

```
[           ][    10    ][    11     ][    12    ][    13    ][    15    ]
[     2     ][  1-UP    ][  2-UP     ][          ][          ][          ]
[     3     ][  1-DOWN  ][  2-DOWN   ][          ][          ][          ]
[     4     ][  1-LEFT  ][  2-LEFT   ][          ][          ][          ]
[     5     ][  1-RIGHT ][  2-RIGHT  ][          ][          ][          ]
[     6     ][  1-FIRE1 ][  2-FIRE1  ][          ][          ][          ]
[     7     ][  1-FIRE2 ][  2-FIRE2  ][          ][          ][          ]
[     8     ][  1-FIRE3 ][  2-FIRE3  ][          ][          ][          ]
[     9     ][  1-FIRE4 ][  2-FIRE4  ][          ][          ][          ]
[     1     ][  1-FIRE5 ][  2-FIRE5  ][          ][          ][          ]
[    14     ][  1-FIRE6 ][  2-FIRE6  ][          ][          ][          ]
[    16     ][  1-FIRE7 ][  2-FIRE7  ][          ][          ][          ]
[    17     ][  1-FIRE8 ][  2-FIRE8  ][          ][          ][          ]
```

In this way the driver only have to look the number of active columns/controls and not to have wait more of the necessary. If you have read the previous links, you will see that the implementation of this driver on arcade control panels is very simple. Here some examples:

```
NOTE: This diagram need an update. See the sketch above.
```

And you could add more joysticks with the next available lines (11,12,13,15). If you dont see yet, you need get attention to the next example with two controls:

Example with two joys on lines 11, 12:

![https://openppjoy.googlecode.com/files/panel-openppjoy.png](https://openppjoy.googlecode.com/files/panel-openppjoy.png)
_diagram simplified just with joy directions_

Finally you can use 5 controls with 8 buttons each or 3 controls and 2 basic configurations of keyboard, etc... it is your choice! :)

# Emulated Keys and buttons #


```
(1) JOYSTICK: 2 axes y 8 buttons.

    KEYBOARD:
	(2) KEY1 - 	        CURSOR_UP      //MAME PLAYER 1 KEYS
		 		CURSOR_DOWN
				CURSOR_LEFT
				CURSOR_RIGHT
				LEFT_CONTROL
				LEFT_ALT
				SPACE
				LEFT_SHIFT
				[Z]
				[X]
				[5]
				[1]

	(3)	KEY2 -	        [R]      //MAME PLAYER 2 KEYS
				[F]
				[D]
				[G]
				[A]
				[S]
				[Q]
				[W]
				[I]
				[K]
				[6]
				[2]

	(4)	KEY3 -	        [N]      //EXTRAS
				[M]
				[P]      //MAME pause
				[L]
				[ENTER]
				[ESC]
				[TAB]
				[GRAVE] //º
				[3]     //3P
				[4]     //4P
				[7]     //COIN 3
				[8]     //COIN 4
```

# Configuration and Use #

To a complete install the driver you need the Linux kernel headers in ubuntu will be:

```
# apt-get install linux-headers-`uname -r`
```

When instalation is complete, do it:

```
# make
```

If the source is compiled ok, now you need configure it. You can use "map=" to adapt the driver to your hardware, the module parameters are:

```
map=x,y1,y2,y3,y4,y5
```

The [x](x.md) defines the paralel port to use, in most cases will be 0.
The [yN](yN.md) defines the number of the control type that you want to use from this list:
1 - 8 Buttons Joystick
2 - Key config 1 (see above)
3 - Key config 2 (see above)

Some final examples, but note that "insmod" loads the module, and "arcade" it is the module (if you dont understood, please search tutorials about modules, kernel... on internet).

```
# insmod arcade map=0,1
```

And this loads a joystick with 8 buttons on line 10...

```
[           ][    10    ][    11     ][    12    ][    13    ][    15    ]
[     2     ][  1-UP    ][           ][          ][          ][          ]
[     3     ][  1-DOWN  ][           ][          ][          ][          ]
....
```

But if you type:

```
# insmod arcade map=0,1,1,1,1,1
```

Would have 5 joysticks of 8 buttons thats is reading the lines: 10,11,12,13,15 :D

```
[           ][    10    ][    11     ][    12    ][    13    ][    15    ]
[     2     ][  1-UP    ][  2-UP     ][  3-UP    ][  4-UP    ][  5-UP    ]
[     3     ][  1-DOWN  ][  2-DOWN   ][  3-DOWN  ][  4-DOWN  ][  5-DOWN  ]
....
```

And finally:

```
# insmod arcade map=0,1,1,1,1,4
```

We can easily get 4 joysticks with 8 buttons and a basic configuration of keyboard

```
[           ][    10    ][    11     ][    12    ][    13    ][    15    ]
[     2     ][  1-UP    ][  2-UP     ][  3-UP    ][  4-UP    ][  1-EXTR1 ]
[     3     ][  1-DOWN  ][  2-DOWN   ][  3-DOWN  ][  4-DOWN  ][  1-EXTR2 ]
....
```