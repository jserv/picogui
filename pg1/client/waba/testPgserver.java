/* $Id$ */

/*
 * testPgserver.java - a small class to test the org.picogui.NetCore
 *                     and associated classes
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Waba/Java client library done by Olivier Bornet, SMARTDATA
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 *
 * 
 * 
 */

import waba.sys.VmShell;

import org.picogui.NetCore;
import org.picogui._pgReqdRegister;

public class testPgserver {

    public static void main (String[] args) {

	NetCore nc = new NetCore ();

	if (nc.pgInit (args)) {

	    VmShell.println ("all is OK");

	} else {

	    VmShell.println ("problem with the connection");

	    return;

	}

	for (short i = 0; i < args.length; i++) {

	    VmShell.println ("test--args[" + i + "] = \"" + args[i] + "\"");

	}

	VmShell.println ("try a ping");
	if (nc.pgPing ()) {

	  VmShell.println ("OK");

	} else {

	  VmShell.println ("BAD");

	}

	VmShell.println ("try register the application");
	int appHandle = nc.pgRegisterApp ("cli_waba", _pgReqdRegister.PG_APP_NORMAL);
	if (appHandle != 0) {

	  VmShell.println ("OK");

	} else {

	  VmShell.println ("BAD");

	}

	VmShell.println ("try an update");
	if (nc.pgUpdate ()) {

	  VmShell.println ("OK");

	} else {

	  VmShell.println ("BAD");

	}

	VmShell.println ("try to create a string");
	byte b[] = { (byte)'s', (byte)'a', (byte)'l', (byte)'u', (byte)'t', 0 };
	int strHandle = nc.pgNewString (b);
	if( strHandle != 0 ) {

	  VmShell.println ("OK (" + strHandle + ")");

	} else {

	  VmShell.println ("BAD");

	}

	VmShell.println ("try to create a 2nd string");
	int strHandle2 = nc.pgNewString ("coucou");
	if( strHandle2 != 0 ) {

	  VmShell.println ("OK (" + strHandle2 + ")");

	} else {

	  VmShell.println ("BAD");

	}

	int i = 0;
	while( i == i ) {
	  i++;
	  if( i % 100000 == 0 ) {
	    VmShell.print (".");
	  }
	}
    }
}
