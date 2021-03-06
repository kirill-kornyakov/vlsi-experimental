/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     K_HISTORY = 258,
     K_ABUT = 259,
     K_ABUTMENT = 260,
     K_ACTIVE = 261,
     K_ANALOG = 262,
     K_ARRAY = 263,
     K_AREA = 264,
     K_BLOCK = 265,
     K_BOTTOMLEFT = 266,
     K_BOTTOMRIGHT = 267,
     K_BY = 268,
     K_CAPACITANCE = 269,
     K_CAPMULTIPLIER = 270,
     K_CLASS = 271,
     K_CLOCK = 272,
     K_CLOCKTYPE = 273,
     K_COLUMNMAJOR = 274,
     K_DESIGNRULEWIDTH = 275,
     K_INFLUENCE = 276,
     K_CORE = 277,
     K_CORNER = 278,
     K_COVER = 279,
     K_CPERSQDIST = 280,
     K_CURRENT = 281,
     K_CURRENTSOURCE = 282,
     K_CUT = 283,
     K_DEFAULT = 284,
     K_DATABASE = 285,
     K_DATA = 286,
     K_DIELECTRIC = 287,
     K_DIRECTION = 288,
     K_DO = 289,
     K_EDGECAPACITANCE = 290,
     K_EEQ = 291,
     K_END = 292,
     K_ENDCAP = 293,
     K_FALL = 294,
     K_FALLCS = 295,
     K_FALLT0 = 296,
     K_FALLSATT1 = 297,
     K_FALLRS = 298,
     K_FALLSATCUR = 299,
     K_FALLTHRESH = 300,
     K_FEEDTHRU = 301,
     K_FIXED = 302,
     K_FOREIGN = 303,
     K_FROMPIN = 304,
     K_GENERATE = 305,
     K_GENERATOR = 306,
     K_GROUND = 307,
     K_HEIGHT = 308,
     K_HORIZONTAL = 309,
     K_INOUT = 310,
     K_INPUT = 311,
     K_INPUTNOISEMARGIN = 312,
     K_COMPONENTPIN = 313,
     K_INTRINSIC = 314,
     K_INVERT = 315,
     K_IRDROP = 316,
     K_ITERATE = 317,
     K_IV_TABLES = 318,
     K_LAYER = 319,
     K_LEAKAGE = 320,
     K_LEQ = 321,
     K_LIBRARY = 322,
     K_MACRO = 323,
     K_MATCH = 324,
     K_MAXDELAY = 325,
     K_MAXLOAD = 326,
     K_METALOVERHANG = 327,
     K_MILLIAMPS = 328,
     K_MILLIWATTS = 329,
     K_MINFEATURE = 330,
     K_MUSTJOIN = 331,
     K_NAMESCASESENSITIVE = 332,
     K_NANOSECONDS = 333,
     K_NETS = 334,
     K_NEW = 335,
     K_NONDEFAULTRULE = 336,
     K_NONINVERT = 337,
     K_NONUNATE = 338,
     K_OBS = 339,
     K_OHMS = 340,
     K_OFFSET = 341,
     K_ORIENTATION = 342,
     K_ORIGIN = 343,
     K_OUTPUT = 344,
     K_OUTPUTNOISEMARGIN = 345,
     K_OUTPUTRESISTANCE = 346,
     K_OVERHANG = 347,
     K_OVERLAP = 348,
     K_OFF = 349,
     K_ON = 350,
     K_OVERLAPS = 351,
     K_PAD = 352,
     K_PATH = 353,
     K_PATTERN = 354,
     K_PICOFARADS = 355,
     K_PIN = 356,
     K_PITCH = 357,
     K_PLACED = 358,
     K_POLYGON = 359,
     K_PORT = 360,
     K_POST = 361,
     K_POWER = 362,
     K_PRE = 363,
     K_PULLDOWNRES = 364,
     K_RECT = 365,
     K_RESISTANCE = 366,
     K_RESISTIVE = 367,
     K_RING = 368,
     K_RISE = 369,
     K_RISECS = 370,
     K_RISERS = 371,
     K_RISESATCUR = 372,
     K_RISETHRESH = 373,
     K_RISESATT1 = 374,
     K_RISET0 = 375,
     K_RISEVOLTAGETHRESHOLD = 376,
     K_FALLVOLTAGETHRESHOLD = 377,
     K_ROUTING = 378,
     K_ROWMAJOR = 379,
     K_RPERSQ = 380,
     K_SAMENET = 381,
     K_SCANUSE = 382,
     K_SHAPE = 383,
     K_SHRINKAGE = 384,
     K_SIGNAL = 385,
     K_SITE = 386,
     K_SIZE = 387,
     K_SOURCE = 388,
     K_SPACER = 389,
     K_SPACING = 390,
     K_SPECIALNETS = 391,
     K_STACK = 392,
     K_START = 393,
     K_STEP = 394,
     K_STOP = 395,
     K_STRUCTURE = 396,
     K_SYMMETRY = 397,
     K_TABLE = 398,
     K_THICKNESS = 399,
     K_TIEHIGH = 400,
     K_TIELOW = 401,
     K_TIEOFFR = 402,
     K_TIME = 403,
     K_TIMING = 404,
     K_TO = 405,
     K_TOPIN = 406,
     K_TOPLEFT = 407,
     K_TOPRIGHT = 408,
     K_TOPOFSTACKONLY = 409,
     K_TRISTATE = 410,
     K_TYPE = 411,
     K_UNATENESS = 412,
     K_UNITS = 413,
     K_USE = 414,
     K_VARIABLE = 415,
     K_VERTICAL = 416,
     K_VHI = 417,
     K_VIA = 418,
     K_VIARULE = 419,
     K_VLO = 420,
     K_VOLTAGE = 421,
     K_VOLTS = 422,
     K_WIDTH = 423,
     K_X = 424,
     K_Y = 425,
     K_R90 = 426,
     T_STRING = 427,
     QSTRING = 428,
     NUMBER = 429,
     K_N = 430,
     K_S = 431,
     K_E = 432,
     K_W = 433,
     K_FN = 434,
     K_FS = 435,
     K_FE = 436,
     K_FW = 437,
     K_R0 = 438,
     K_R180 = 439,
     K_R270 = 440,
     K_MX = 441,
     K_MY = 442,
     K_MXR90 = 443,
     K_MYR90 = 444,
     K_USER = 445,
     K_MASTERSLICE = 446,
     K_ENDMACRO = 447,
     K_ENDMACROPIN = 448,
     K_ENDVIARULE = 449,
     K_ENDVIA = 450,
     K_ENDLAYER = 451,
     K_ENDSITE = 452,
     K_CANPLACE = 453,
     K_CANNOTOCCUPY = 454,
     K_TRACKS = 455,
     K_FLOORPLAN = 456,
     K_GCELLGRID = 457,
     K_DEFAULTCAP = 458,
     K_MINPINS = 459,
     K_WIRECAP = 460,
     K_STABLE = 461,
     K_SETUP = 462,
     K_HOLD = 463,
     K_DEFINE = 464,
     K_DEFINES = 465,
     K_DEFINEB = 466,
     K_IF = 467,
     K_THEN = 468,
     K_ELSE = 469,
     K_FALSE = 470,
     K_TRUE = 471,
     K_EQ = 472,
     K_NE = 473,
     K_LE = 474,
     K_LT = 475,
     K_GE = 476,
     K_GT = 477,
     K_OR = 478,
     K_AND = 479,
     K_NOT = 480,
     K_DELAY = 481,
     K_TABLEDIMENSION = 482,
     K_TABLEAXIS = 483,
     K_TABLEENTRIES = 484,
     K_TRANSITIONTIME = 485,
     K_EXTENSION = 486,
     K_PROPDEF = 487,
     K_STRING = 488,
     K_INTEGER = 489,
     K_REAL = 490,
     K_RANGE = 491,
     K_PROPERTY = 492,
     K_VIRTUAL = 493,
     K_BUSBITCHARS = 494,
     K_VERSION = 495,
     K_BEGINEXT = 496,
     K_ENDEXT = 497,
     K_UNIVERSALNOISEMARGIN = 498,
     K_EDGERATETHRESHOLD1 = 499,
     K_CORRECTIONTABLE = 500,
     K_EDGERATESCALEFACTOR = 501,
     K_EDGERATETHRESHOLD2 = 502,
     K_VICTIMNOISE = 503,
     K_NOISETABLE = 504,
     K_EDGERATE = 505,
     K_VICTIMLENGTH = 506,
     K_CORRECTIONFACTOR = 507,
     K_OUTPUTPINANTENNASIZE = 508,
     K_INPUTPINANTENNASIZE = 509,
     K_INOUTPINANTENNASIZE = 510,
     K_CURRENTDEN = 511,
     K_PWL = 512,
     K_ANTENNALENGTHFACTOR = 513,
     K_TAPERRULE = 514,
     K_DIVIDERCHAR = 515,
     K_ANTENNASIZE = 516,
     K_ANTENNAMETALLENGTH = 517,
     K_ANTENNAMETALAREA = 518,
     K_RISESLEWLIMIT = 519,
     K_FALLSLEWLIMIT = 520,
     K_FUNCTION = 521,
     K_BUFFER = 522,
     K_INVERTER = 523,
     K_NAMEMAPSTRING = 524,
     K_NOWIREEXTENSIONATPIN = 525,
     K_WIREEXTENSION = 526,
     K_MESSAGE = 527,
     K_CREATEFILE = 528,
     K_OPENFILE = 529,
     K_CLOSEFILE = 530,
     K_WARNING = 531,
     K_ERROR = 532,
     K_FATALERROR = 533,
     K_RECOVERY = 534,
     K_SKEW = 535,
     K_ANYEDGE = 536,
     K_POSEDGE = 537,
     K_NEGEDGE = 538,
     K_SDFCONDSTART = 539,
     K_SDFCONDEND = 540,
     K_SDFCOND = 541,
     K_MPWH = 542,
     K_MPWL = 543,
     K_PERIOD = 544,
     K_ACCURRENTDENSITY = 545,
     K_DCCURRENTDENSITY = 546,
     K_AVERAGE = 547,
     K_PEAK = 548,
     K_RMS = 549,
     K_FREQUENCY = 550,
     K_CUTAREA = 551,
     K_MEGAHERTZ = 552,
     K_USELENGTHTHRESHOLD = 553,
     K_LENGTHTHRESHOLD = 554,
     K_ANTENNAINPUTGATEAREA = 555,
     K_ANTENNAINOUTDIFFAREA = 556,
     K_ANTENNAOUTPUTDIFFAREA = 557,
     K_ANTENNAAREARATIO = 558,
     K_ANTENNADIFFAREARATIO = 559,
     K_ANTENNACUMAREARATIO = 560,
     K_ANTENNACUMDIFFAREARATIO = 561,
     K_ANTENNAAREAFACTOR = 562,
     K_ANTENNASIDEAREARATIO = 563,
     K_ANTENNADIFFSIDEAREARATIO = 564,
     K_ANTENNACUMSIDEAREARATIO = 565,
     K_ANTENNACUMDIFFSIDEAREARATIO = 566,
     K_ANTENNASIDEAREAFACTOR = 567,
     K_DIFFUSEONLY = 568,
     K_MANUFACTURINGGRID = 569,
     K_ANTENNACELL = 570,
     K_CLEARANCEMEASURE = 571,
     K_EUCLIDEAN = 572,
     K_MAXXY = 573,
     K_USEMINSPACING = 574,
     K_ROWMINSPACING = 575,
     K_ROWABUTSPACING = 576,
     K_FLIP = 577,
     K_NONE = 578,
     K_ANTENNAPARTIALMETALAREA = 579,
     K_ANTENNAPARTIALMETALSIDEAREA = 580,
     K_ANTENNAGATEAREA = 581,
     K_ANTENNADIFFAREA = 582,
     K_ANTENNAMAXAREACAR = 583,
     K_ANTENNAMAXSIDEAREACAR = 584,
     K_ANTENNAPARTIALCUTAREA = 585,
     K_ANTENNAMAXCUTCAR = 586,
     K_SLOTWIREWIDTH = 587,
     K_SLOTWIRELENGTH = 588,
     K_SLOTWIDTH = 589,
     K_SLOTLENGTH = 590,
     K_MAXADJACENTSLOTSPACING = 591,
     K_MAXCOAXIALSLOTSPACING = 592,
     K_MAXEDGESLOTSPACING = 593,
     K_SPLITWIREWIDTH = 594,
     K_MINIMUMDENSITY = 595,
     K_MAXIMUMDENSITY = 596,
     K_DENSITYCHECKWINDOW = 597,
     K_DENSITYCHECKSTEP = 598,
     K_FILLACTIVESPACING = 599,
     K_MINIMUMCUT = 600,
     K_ADJACENTCUTS = 601,
     K_ANTENNAMODEL = 602,
     K_BUMP = 603,
     K_ENCLOSURE = 604,
     K_FROMABOVE = 605,
     K_FROMBELOW = 606,
     K_IMPLANT = 607,
     K_LENGTH = 608,
     K_MAXVIASTACK = 609,
     K_AREAIO = 610,
     K_BLACKBOX = 611,
     K_MAXWIDTH = 612,
     K_MINENCLOSEDAREA = 613,
     K_MINSTEP = 614,
     K_ORIENT = 615,
     K_OXIDE1 = 616,
     K_OXIDE2 = 617,
     K_OXIDE3 = 618,
     K_OXIDE4 = 619,
     K_PARALLELRUNLENGTH = 620,
     K_MINWIDTH = 621,
     K_PROTRUSIONWIDTH = 622,
     K_SPACINGTABLE = 623,
     K_WITHIN = 624,
     K_ABOVE = 625,
     K_BELOW = 626,
     K_CENTERTOCENTER = 627,
     K_CUTSIZE = 628,
     K_CUTSPACING = 629,
     K_DENSITY = 630,
     K_DIAG45 = 631,
     K_DIAG135 = 632,
     K_DIAGMINEDGELENGTH = 633,
     K_DIAGSPACING = 634,
     K_DIAGPITCH = 635,
     K_DIAGWIDTH = 636,
     K_GENERATED = 637,
     K_GROUNDSENSITIVITY = 638,
     K_HARDSPACING = 639,
     K_INSIDECORNER = 640,
     K_LAYERS = 641,
     K_LENGTHSUM = 642,
     K_MICRONS = 643,
     K_MINCUTS = 644,
     K_MINSIZE = 645,
     K_NETEXPR = 646,
     K_OUTSIDECORNER = 647,
     K_PREFERENCLOSURE = 648,
     K_ROWCOL = 649,
     K_ROWPATTERN = 650,
     K_SOFT = 651,
     K_SUPPLYSENSITIVITY = 652,
     K_USEVIA = 653,
     K_USEVIARULE = 654,
     K_WELLTAP = 655,
     K_ARRAYCUTS = 656,
     K_ARRAYSPACING = 657,
     K_ANTENNAAREADIFFREDUCEPWL = 658,
     K_ANTENNAAREAMINUSDIFF = 659,
     K_ANTENNACUMROUTINGPLUSCUT = 660,
     K_ANTENNAGATEPLUSDIFF = 661,
     K_ENDOFLINE = 662,
     K_ENDOFNOTCHWIDTH = 663,
     K_EXCEPTEXTRACUT = 664,
     K_EXCEPTSAMEPGNET = 665,
     K_EXCEPTPGNET = 666,
     K_LONGARRAY = 667,
     K_MAXEDGES = 668,
     K_NOTCHLENGTH = 669,
     K_NOTCHSPACING = 670,
     K_ORTHOGONAL = 671,
     K_PARALLELEDGE = 672,
     K_PARALLELOVERLAP = 673,
     K_PGONLY = 674,
     K_PRL = 675,
     K_TWOEDGES = 676,
     K_TWOWIDTHS = 677,
     IF = 678,
     LNOT = 679,
     UMINUS = 680
   };
#endif
/* Tokens.  */
#define K_HISTORY 258
#define K_ABUT 259
#define K_ABUTMENT 260
#define K_ACTIVE 261
#define K_ANALOG 262
#define K_ARRAY 263
#define K_AREA 264
#define K_BLOCK 265
#define K_BOTTOMLEFT 266
#define K_BOTTOMRIGHT 267
#define K_BY 268
#define K_CAPACITANCE 269
#define K_CAPMULTIPLIER 270
#define K_CLASS 271
#define K_CLOCK 272
#define K_CLOCKTYPE 273
#define K_COLUMNMAJOR 274
#define K_DESIGNRULEWIDTH 275
#define K_INFLUENCE 276
#define K_CORE 277
#define K_CORNER 278
#define K_COVER 279
#define K_CPERSQDIST 280
#define K_CURRENT 281
#define K_CURRENTSOURCE 282
#define K_CUT 283
#define K_DEFAULT 284
#define K_DATABASE 285
#define K_DATA 286
#define K_DIELECTRIC 287
#define K_DIRECTION 288
#define K_DO 289
#define K_EDGECAPACITANCE 290
#define K_EEQ 291
#define K_END 292
#define K_ENDCAP 293
#define K_FALL 294
#define K_FALLCS 295
#define K_FALLT0 296
#define K_FALLSATT1 297
#define K_FALLRS 298
#define K_FALLSATCUR 299
#define K_FALLTHRESH 300
#define K_FEEDTHRU 301
#define K_FIXED 302
#define K_FOREIGN 303
#define K_FROMPIN 304
#define K_GENERATE 305
#define K_GENERATOR 306
#define K_GROUND 307
#define K_HEIGHT 308
#define K_HORIZONTAL 309
#define K_INOUT 310
#define K_INPUT 311
#define K_INPUTNOISEMARGIN 312
#define K_COMPONENTPIN 313
#define K_INTRINSIC 314
#define K_INVERT 315
#define K_IRDROP 316
#define K_ITERATE 317
#define K_IV_TABLES 318
#define K_LAYER 319
#define K_LEAKAGE 320
#define K_LEQ 321
#define K_LIBRARY 322
#define K_MACRO 323
#define K_MATCH 324
#define K_MAXDELAY 325
#define K_MAXLOAD 326
#define K_METALOVERHANG 327
#define K_MILLIAMPS 328
#define K_MILLIWATTS 329
#define K_MINFEATURE 330
#define K_MUSTJOIN 331
#define K_NAMESCASESENSITIVE 332
#define K_NANOSECONDS 333
#define K_NETS 334
#define K_NEW 335
#define K_NONDEFAULTRULE 336
#define K_NONINVERT 337
#define K_NONUNATE 338
#define K_OBS 339
#define K_OHMS 340
#define K_OFFSET 341
#define K_ORIENTATION 342
#define K_ORIGIN 343
#define K_OUTPUT 344
#define K_OUTPUTNOISEMARGIN 345
#define K_OUTPUTRESISTANCE 346
#define K_OVERHANG 347
#define K_OVERLAP 348
#define K_OFF 349
#define K_ON 350
#define K_OVERLAPS 351
#define K_PAD 352
#define K_PATH 353
#define K_PATTERN 354
#define K_PICOFARADS 355
#define K_PIN 356
#define K_PITCH 357
#define K_PLACED 358
#define K_POLYGON 359
#define K_PORT 360
#define K_POST 361
#define K_POWER 362
#define K_PRE 363
#define K_PULLDOWNRES 364
#define K_RECT 365
#define K_RESISTANCE 366
#define K_RESISTIVE 367
#define K_RING 368
#define K_RISE 369
#define K_RISECS 370
#define K_RISERS 371
#define K_RISESATCUR 372
#define K_RISETHRESH 373
#define K_RISESATT1 374
#define K_RISET0 375
#define K_RISEVOLTAGETHRESHOLD 376
#define K_FALLVOLTAGETHRESHOLD 377
#define K_ROUTING 378
#define K_ROWMAJOR 379
#define K_RPERSQ 380
#define K_SAMENET 381
#define K_SCANUSE 382
#define K_SHAPE 383
#define K_SHRINKAGE 384
#define K_SIGNAL 385
#define K_SITE 386
#define K_SIZE 387
#define K_SOURCE 388
#define K_SPACER 389
#define K_SPACING 390
#define K_SPECIALNETS 391
#define K_STACK 392
#define K_START 393
#define K_STEP 394
#define K_STOP 395
#define K_STRUCTURE 396
#define K_SYMMETRY 397
#define K_TABLE 398
#define K_THICKNESS 399
#define K_TIEHIGH 400
#define K_TIELOW 401
#define K_TIEOFFR 402
#define K_TIME 403
#define K_TIMING 404
#define K_TO 405
#define K_TOPIN 406
#define K_TOPLEFT 407
#define K_TOPRIGHT 408
#define K_TOPOFSTACKONLY 409
#define K_TRISTATE 410
#define K_TYPE 411
#define K_UNATENESS 412
#define K_UNITS 413
#define K_USE 414
#define K_VARIABLE 415
#define K_VERTICAL 416
#define K_VHI 417
#define K_VIA 418
#define K_VIARULE 419
#define K_VLO 420
#define K_VOLTAGE 421
#define K_VOLTS 422
#define K_WIDTH 423
#define K_X 424
#define K_Y 425
#define K_R90 426
#define T_STRING 427
#define QSTRING 428
#define NUMBER 429
#define K_N 430
#define K_S 431
#define K_E 432
#define K_W 433
#define K_FN 434
#define K_FS 435
#define K_FE 436
#define K_FW 437
#define K_R0 438
#define K_R180 439
#define K_R270 440
#define K_MX 441
#define K_MY 442
#define K_MXR90 443
#define K_MYR90 444
#define K_USER 445
#define K_MASTERSLICE 446
#define K_ENDMACRO 447
#define K_ENDMACROPIN 448
#define K_ENDVIARULE 449
#define K_ENDVIA 450
#define K_ENDLAYER 451
#define K_ENDSITE 452
#define K_CANPLACE 453
#define K_CANNOTOCCUPY 454
#define K_TRACKS 455
#define K_FLOORPLAN 456
#define K_GCELLGRID 457
#define K_DEFAULTCAP 458
#define K_MINPINS 459
#define K_WIRECAP 460
#define K_STABLE 461
#define K_SETUP 462
#define K_HOLD 463
#define K_DEFINE 464
#define K_DEFINES 465
#define K_DEFINEB 466
#define K_IF 467
#define K_THEN 468
#define K_ELSE 469
#define K_FALSE 470
#define K_TRUE 471
#define K_EQ 472
#define K_NE 473
#define K_LE 474
#define K_LT 475
#define K_GE 476
#define K_GT 477
#define K_OR 478
#define K_AND 479
#define K_NOT 480
#define K_DELAY 481
#define K_TABLEDIMENSION 482
#define K_TABLEAXIS 483
#define K_TABLEENTRIES 484
#define K_TRANSITIONTIME 485
#define K_EXTENSION 486
#define K_PROPDEF 487
#define K_STRING 488
#define K_INTEGER 489
#define K_REAL 490
#define K_RANGE 491
#define K_PROPERTY 492
#define K_VIRTUAL 493
#define K_BUSBITCHARS 494
#define K_VERSION 495
#define K_BEGINEXT 496
#define K_ENDEXT 497
#define K_UNIVERSALNOISEMARGIN 498
#define K_EDGERATETHRESHOLD1 499
#define K_CORRECTIONTABLE 500
#define K_EDGERATESCALEFACTOR 501
#define K_EDGERATETHRESHOLD2 502
#define K_VICTIMNOISE 503
#define K_NOISETABLE 504
#define K_EDGERATE 505
#define K_VICTIMLENGTH 506
#define K_CORRECTIONFACTOR 507
#define K_OUTPUTPINANTENNASIZE 508
#define K_INPUTPINANTENNASIZE 509
#define K_INOUTPINANTENNASIZE 510
#define K_CURRENTDEN 511
#define K_PWL 512
#define K_ANTENNALENGTHFACTOR 513
#define K_TAPERRULE 514
#define K_DIVIDERCHAR 515
#define K_ANTENNASIZE 516
#define K_ANTENNAMETALLENGTH 517
#define K_ANTENNAMETALAREA 518
#define K_RISESLEWLIMIT 519
#define K_FALLSLEWLIMIT 520
#define K_FUNCTION 521
#define K_BUFFER 522
#define K_INVERTER 523
#define K_NAMEMAPSTRING 524
#define K_NOWIREEXTENSIONATPIN 525
#define K_WIREEXTENSION 526
#define K_MESSAGE 527
#define K_CREATEFILE 528
#define K_OPENFILE 529
#define K_CLOSEFILE 530
#define K_WARNING 531
#define K_ERROR 532
#define K_FATALERROR 533
#define K_RECOVERY 534
#define K_SKEW 535
#define K_ANYEDGE 536
#define K_POSEDGE 537
#define K_NEGEDGE 538
#define K_SDFCONDSTART 539
#define K_SDFCONDEND 540
#define K_SDFCOND 541
#define K_MPWH 542
#define K_MPWL 543
#define K_PERIOD 544
#define K_ACCURRENTDENSITY 545
#define K_DCCURRENTDENSITY 546
#define K_AVERAGE 547
#define K_PEAK 548
#define K_RMS 549
#define K_FREQUENCY 550
#define K_CUTAREA 551
#define K_MEGAHERTZ 552
#define K_USELENGTHTHRESHOLD 553
#define K_LENGTHTHRESHOLD 554
#define K_ANTENNAINPUTGATEAREA 555
#define K_ANTENNAINOUTDIFFAREA 556
#define K_ANTENNAOUTPUTDIFFAREA 557
#define K_ANTENNAAREARATIO 558
#define K_ANTENNADIFFAREARATIO 559
#define K_ANTENNACUMAREARATIO 560
#define K_ANTENNACUMDIFFAREARATIO 561
#define K_ANTENNAAREAFACTOR 562
#define K_ANTENNASIDEAREARATIO 563
#define K_ANTENNADIFFSIDEAREARATIO 564
#define K_ANTENNACUMSIDEAREARATIO 565
#define K_ANTENNACUMDIFFSIDEAREARATIO 566
#define K_ANTENNASIDEAREAFACTOR 567
#define K_DIFFUSEONLY 568
#define K_MANUFACTURINGGRID 569
#define K_ANTENNACELL 570
#define K_CLEARANCEMEASURE 571
#define K_EUCLIDEAN 572
#define K_MAXXY 573
#define K_USEMINSPACING 574
#define K_ROWMINSPACING 575
#define K_ROWABUTSPACING 576
#define K_FLIP 577
#define K_NONE 578
#define K_ANTENNAPARTIALMETALAREA 579
#define K_ANTENNAPARTIALMETALSIDEAREA 580
#define K_ANTENNAGATEAREA 581
#define K_ANTENNADIFFAREA 582
#define K_ANTENNAMAXAREACAR 583
#define K_ANTENNAMAXSIDEAREACAR 584
#define K_ANTENNAPARTIALCUTAREA 585
#define K_ANTENNAMAXCUTCAR 586
#define K_SLOTWIREWIDTH 587
#define K_SLOTWIRELENGTH 588
#define K_SLOTWIDTH 589
#define K_SLOTLENGTH 590
#define K_MAXADJACENTSLOTSPACING 591
#define K_MAXCOAXIALSLOTSPACING 592
#define K_MAXEDGESLOTSPACING 593
#define K_SPLITWIREWIDTH 594
#define K_MINIMUMDENSITY 595
#define K_MAXIMUMDENSITY 596
#define K_DENSITYCHECKWINDOW 597
#define K_DENSITYCHECKSTEP 598
#define K_FILLACTIVESPACING 599
#define K_MINIMUMCUT 600
#define K_ADJACENTCUTS 601
#define K_ANTENNAMODEL 602
#define K_BUMP 603
#define K_ENCLOSURE 604
#define K_FROMABOVE 605
#define K_FROMBELOW 606
#define K_IMPLANT 607
#define K_LENGTH 608
#define K_MAXVIASTACK 609
#define K_AREAIO 610
#define K_BLACKBOX 611
#define K_MAXWIDTH 612
#define K_MINENCLOSEDAREA 613
#define K_MINSTEP 614
#define K_ORIENT 615
#define K_OXIDE1 616
#define K_OXIDE2 617
#define K_OXIDE3 618
#define K_OXIDE4 619
#define K_PARALLELRUNLENGTH 620
#define K_MINWIDTH 621
#define K_PROTRUSIONWIDTH 622
#define K_SPACINGTABLE 623
#define K_WITHIN 624
#define K_ABOVE 625
#define K_BELOW 626
#define K_CENTERTOCENTER 627
#define K_CUTSIZE 628
#define K_CUTSPACING 629
#define K_DENSITY 630
#define K_DIAG45 631
#define K_DIAG135 632
#define K_DIAGMINEDGELENGTH 633
#define K_DIAGSPACING 634
#define K_DIAGPITCH 635
#define K_DIAGWIDTH 636
#define K_GENERATED 637
#define K_GROUNDSENSITIVITY 638
#define K_HARDSPACING 639
#define K_INSIDECORNER 640
#define K_LAYERS 641
#define K_LENGTHSUM 642
#define K_MICRONS 643
#define K_MINCUTS 644
#define K_MINSIZE 645
#define K_NETEXPR 646
#define K_OUTSIDECORNER 647
#define K_PREFERENCLOSURE 648
#define K_ROWCOL 649
#define K_ROWPATTERN 650
#define K_SOFT 651
#define K_SUPPLYSENSITIVITY 652
#define K_USEVIA 653
#define K_USEVIARULE 654
#define K_WELLTAP 655
#define K_ARRAYCUTS 656
#define K_ARRAYSPACING 657
#define K_ANTENNAAREADIFFREDUCEPWL 658
#define K_ANTENNAAREAMINUSDIFF 659
#define K_ANTENNACUMROUTINGPLUSCUT 660
#define K_ANTENNAGATEPLUSDIFF 661
#define K_ENDOFLINE 662
#define K_ENDOFNOTCHWIDTH 663
#define K_EXCEPTEXTRACUT 664
#define K_EXCEPTSAMEPGNET 665
#define K_EXCEPTPGNET 666
#define K_LONGARRAY 667
#define K_MAXEDGES 668
#define K_NOTCHLENGTH 669
#define K_NOTCHSPACING 670
#define K_ORTHOGONAL 671
#define K_PARALLELEDGE 672
#define K_PARALLELOVERLAP 673
#define K_PGONLY 674
#define K_PRL 675
#define K_TWOEDGES 676
#define K_TWOWIDTHS 677
#define IF 678
#define LNOT 679
#define UMINUS 680




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 368 "lef.y"
typedef union YYSTYPE {
	double    dval ;
	int       integer ;
	char *    string ;
	lefPOINT  pt;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 895 "lef.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE lefyylval;



