Shorten play (shnplay) 3.6.1-10
=======================

Introduction
------------
	This is a Shorten player plugin for the PM123.

	Shorten is an audio compressor (lossy and lossless)
	originally written by Tony Robinson at SoftSound Ltd.

Features
--------
	Based on shorten-3.6.1 from http://etree.org/shnutils/shorten/
	Seek tables (either appended or .skt standalone) are supported.

System requirements
-------------------
	OS/2 Warp 3 and up
	PM123 1.31 or later,
		http://glass.os2.spb.ru/software/english/pm123.html
	libc063.dll (latest known at the moment is from 
    	   	ftp://ftp.netlabs.org/pub/libc/libc-0_6_3-csd3.exe)

Warnings and limitations
------------------------
	This version is not hardly tested. Use with care!
	VBR calculation is not a time-accurate (ahead on size of audio buffer).
	Only shortens which have RIFF wave original (most common case?) are tested.
	Forward seek is time-consuming in shortens without seek tables
	(seek table can be created with "shorten -s" or "shorten -S").

Installation
------------
	- make it sure that libc063.dll located somewhere in your LIBPATH
	- place the file shnplay.dll into the directory where PM123.EXE located
	- start PM123
	- Right-click on the PM123 window to open the "properties" dialog
	- Choose the page "plugins"
	- Press the "add" button in the "decoder plugin" section
	- Choose "shnplay.dll" in the file dialog.
	  Press Ok.
	- Close "PM123 properties" dialog

	Now you can listen Shorten compressed (.shn) audio files!

De-Installation
---------------
	In case of any trouble with this plugin close PM123 and remove 
	shnplay.dll from the PM123.EXE directory.

License
-------
	This work is derived from shorten compressor, maintained by 
	Jason Jordan <shnutils@freeshell.org> at http://etree.org/shnutils/shorten/
	Declared list of authors:
	   Tony Robinson <ajr@softsound.com>
	   Seek extensions by Wayne Stielau <wstielau@socal.rr.com>
	   Unix hacks and maintenance by Jason Jordan <shnutils@freeshell.org>
	   AIFF support by Brian Willoughby <shorten@sounds.wa.com> of Sound Consulting <http://sounds.wa.com/>
	See doc/COPYING for license terms.

	Based on pm123 plug-ins source, copyrighted by:
	 * Copyright 2004-2007 Dmitry A.Steklenev<glass@ptv.ru>
	 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
	 * Copyright 1997-2003 Taneli Lepp„ <rosmo@sektori.com>
	see doc/COPYRIGHT.html for license terms.

	APE and ID3V1 tags recognition code (id3tag.cpp) is from:
	 * Musepack audio compression
	 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf,
	licensed under the terms of the GNU LGPL 2.1 or later,
	see doc/COPYING.LGPL for full terms.

	Other code, modification and compilation copyrighted by:
	 * Copyright (C) 2008 ntim <ntim@softhome.net>
	and licensed under the GNU GPL version 3 or later,
	see doc/COPYING.GPL for full terms.
	
	The author is not responsible for any damage this program may cause.

Sources
-------
	To meet GNU GPL license terms, all sources attached in src/ directory.
	
	src/src and src/include - contain reworked part of shorten distribution 
	from http://etree.org/shnutils/shorten/dist/src/shorten-3.6.1.tar.gz,
	with compress code removed, platform independancy dropped and
	C++ addiction infected.
	Plag-in code itself located in src/ directory.
	PM123 PDK and a pile of GNU developer tools are need to remake.

Contacts
--------
	All questions about this build please send to ntim@softhome.net
	or contact ntim on #os2russian (irc://efnet/os2russian)

2008, ntim
