/* $Id$
 *
 * NetCore.java - core networking code for the Waba/Java client library
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

package org.picogui;

import waba.io.Socket;
import waba.io.File;
import waba.sys.VmShell;
import waba.sys.Convert;

/**
 * _pgHello class : to store the ident sent by pgserver
 */
public class _pgHello {

    /* the given magic for a pgserver */
    static final int PG_REQUEST_MAGIC = 0x31415926;

    /* the given current version of the implemented protocol */
    static final short PG_PROTOCOL_VER = 0x000C;

    /* magic value saying : yes, I'm pgserver. From the pgserver answer */
    int magic;

    /* the version of the server. From the pgserver answer */
    short protover;

    /* the size to read from the server for the hello ident */
    static final short SIZE = 8; /* int magic + short protover + short dummy */

  /**
   * return the size of the _pgHello component to read
   */
  public int size () {

    return SIZE;

  }

  
    /**
     * Constructor : set the _pgHello variables with the given values
     */
    public _pgHello ( byte values[] ) {

	/* construct the correct magic an protover */
	magic = 0x1000000 * (values[0] & 0xFF) + 0x10000 * (values[1] & 0xFF) 
	        + 0x100 * (values[2] & 0xFF) + (values[3] & 0xFF);
	protover = (short)(0x100 * (values[4] & 0xFF) + (values[5] & 0xFF));

	if (magic != PG_REQUEST_MAGIC) {

	    /* not the correct magic ??? */
	    _pgUtils._client_warn ("Magic not correct (want " + PG_REQUEST_MAGIC + ", get " + magic + ")");

	}

	if (protover != PG_PROTOCOL_VER) {

	    /* not the correct protocol ??? */
	    _pgUtils._client_warn ("Protocol not correct (want " + PG_PROTOCOL_VER + ", get " + protover + ")");

	}

    }

    /**
     * display the information we have
     */
    public void display() {

	_pgUtils._client_msg ("Magic            : " + magic + " (must be : " + PG_REQUEST_MAGIC + ")");
	_pgUtils._client_msg ("Protocol Version : " + protover + " (must be : " + PG_PROTOCOL_VER + ")");

    }

}


/**
 * _pgReqdRegister : class to handle the registration data for an application
 */

public class _pgReqdRegister {

  int name;      /* the handle to the string name of the application */
  short type;    /* the type of application, one of the PG_APP_* constant */

  /* the size to read from the server for the hello ident */
  static final short SIZE = 8; /* int name + short type + short dummy */

  /**
   * return the size of the data
   */
  public int size () {

    return SIZE;

  }

  public static final short PG_APP_NORMAL  = 1; /* Normal application assigned a resizeable window */
  public static final short PG_APP_TOOLBAR = 2; /* Toolbar application using a fixed width window without panelbar */

  /* return the info in a byte array */
  public byte[] inBytes () {

    /* create the basic array with the id, size and type */
    byte b[] = {
      (byte)((name / 0x1000000) % 0x100), (byte)((name / 0x10000) % 0x100),
      (byte)((name / 0x100) % 0x100), (byte)(name % 0x100),
      (byte)(type / 0x100), (byte)(type % 0x100),
      0, 0
    };

    /* return the basic informations */
    return b;

  }

  /**
   * construct with the given informations
   */
  public _pgReqdRegister (int theName, short theType) {

    name = theName;
    type = theType;

  }
}

/**
 * _pgRequest : class to handle the requests sent to pgserver
 */
public class _pgRequest {

  int id;            /* the id of the request */
  int size;          /* the size of the request */
  short type;        /* the type of the request */
  byte data[];       /* data attached to the request */

  /* the size to read from the server for the hello ident */
  static final short SIZE = 12; /* int id + int size + short type + short dummy */

  /* constructor with no additionnal data */
  public _pgRequest (int theID, short theType) {

    id = theID;
    type = theType;
    size = 0;

  }

  /* constructor _with_ additionnal data */
  public _pgRequest (int theID, short theType, byte b[]) {

    id = theID;
    type = theType;
    size = b.length;
    data = b;

  }

  /* return the info in a byte array */
  public byte[] inBytes () {

    /* create the basic array with the id, size and type */
    byte b[] = {
      (byte)((id / 0x1000000) % 0x100), (byte)((id / 0x10000) % 0x100),
      (byte)((id / 0x100) % 0x100), (byte)(id % 0x100),
      (byte)((size / 0x1000000) % 0x100), (byte)((size / 0x10000) % 0x100),
      (byte)((size / 0x100) % 0x100), (byte)(size % 0x100),
      (byte)(type / 0x100), (byte)(type % 0x100),
      0, 0
    };

    if (size > 0) {

      /* OK, we have data attached, so add it to the sent packets */

      byte full_b[] = new byte[ b.length + size ];

      /* copy back the basic informations */
      for (int i =0; i < b.length; i++) {

	full_b[i] = b[i];

      }

      /* add the attached data */
      for (int i = 0; i < size; i++) {

	full_b[b.length + i] = data[i];

      }

      /* return the full infos */
      return full_b;

    } else {

      /* return the basic informations */
      return b;

    }
  }

  /**
   * return the size of the request
   */
  public int size () {

    return SIZE + size;

  }

  static final short PGREQ_PING         = 0; /* Simply return if server is ok */
  static final short PGREQ_UPDATE       = 1; /* Call update() */
  static final short PGREQ_MKWIDGET     = 2; /* Make a widget, return handle */
  static final short PGREQ_MKBITMAP     = 3; /* Make a bitmap, return handle */
  static final short PGREQ_MKFONT       = 4; /* Make a fontdesc, return handle */
  static final short PGREQ_MKSTRING     = 5; /* Make a string, return handle */
  static final short PGREQ_FREE         = 6; /* Free a handle */
  static final short PGREQ_SET          = 7; /* Set a widget param */
  static final short PGREQ_GET          = 8; /* Get a widget param, return it */
  static final short PGREQ_MKTHEME      = 9; /* Load a compiled theme */
  static final short PGREQ_IN_KEY       = 10; /* Dispatch keyboard input */
  static final short PGREQ_IN_POINT     = 11; /* Dispatch pointing device input */
  static final short PGREQ_WAIT         = 13; /* Wait for an event */
  static final short PGREQ_MKFILLSTYLE  = 14; /* Load a fill style, return handle */
  static final short PGREQ_REGISTER     = 15; /* Register a new application */
  static final short PGREQ_MKPOPUP      = 16; /* Create a popup root widget */
  static final short PGREQ_SIZETEXT     = 17; /* Find the size of text */
  static final short PGREQ_BATCH        = 18; /* Execute many requests */
  static final short PGREQ_REGOWNER     = 19; /* Get exclusive privileges */
  static final short PGREQ_UNREGOWNER   = 20; /* Give up exclusive privileges */
  static final short PGREQ_SETMODE      = 21; /* Set video mode/depth/rotation */
  static final short PGREQ_GETMODE      = 22; /* Return a modeinfo struct */
  static final short PGREQ_MKCONTEXT    = 23; /* Enter a new context */
  static final short PGREQ_RMCONTEXT    = 24; /* Clean up and kills the context */
  static final short PGREQ_FOCUS        = 25; /* Force focus to specified widget */
  static final short PGREQ_GETSTRING    = 26; /* Return a RESPONSE_DATA */
  static final short PGREQ_DUP          = 27; /* Duplicate an object */
  static final short PGREQ_SETPAYLOAD   = 28; /* Set an object's payload */
  static final short PGREQ_GETPAYLOAD   = 29; /* Get an object's payload */
  static final short PGREQ_CHCONTEXT    = 30; /* Change a handle's context */
  static final short PGREQ_WRITETO      = 31; /* Stream data to a widget */
  static final short PGREQ_UPDATEPART   = 32; /* Update subtree defined by wgt */
  static final short PGREQ_MKARRAY      = 33; /* Make a array, return handle */
  static final short PGREQ_RENDER       = 34; /* Render gropnode(s) to a bitmap */
  static final short PGREQ_NEWBITMAP    = 35; /* Create a blank bitmap */
  static final short PGREQ_THLOOKUP     = 36; /* Perform a theme lookup */
  static final short PGREQ_GETINACTIVE  = 37; /* Get milliseconds of inactivity */
  static final short PGREQ_SETINACTIVE  = 38; /* Set milliseconds of inactivity */
  static final short PGREQ_DRIVERMSG    = 39; /* Send a message to all drivers */
  static final short PGREQ_LOADDRIVER   = 40; /* Load input/misc (not video) */
  static final short PGREQ_GETFSTYLE    = 41; /* Get info on a font style */
  static final short PGREQ_FINDWIDGET   = 42; /* Get widget handle by name */
  static final short PGREQ_CHECKEVENT   = 43; /* Return number of queued events */
  static final short PGREQ_SIZEBITMAP   = 44; /* Find the size of a bitmap */
  static final short PGREQ_APPMSG       = 45; /* Send PG_WE_APPMSG to any widget */
  static final short PGREQ_CREATEWIDGET = 46; /* Create widget */
  static final short PGREQ_ATTACHWIDGET = 47; /* Attach widget */
  static final short PGREQ_FINDTHOBJ    = 48; /* Find theme object by name */
  static final short PGREQ_TRAVERSEWGT  = 49; /* Find widgets after this one */

}

/**
 * _pgResponse : class to handle all the response of pgserver
 */
public class _pgResponse {

  short type;   /* the type of the response */
  short info1;  /* the infox values are used depending of the type of the response */
  short info2;
  short info3;
  short info4;
  short info5;

  static final short PG_RESPONSE_ERR   = 1; /* response is a pgresponse_err struct */
  static final short PG_RESPONSE_RET   = 2; /* response is a pgresponse_ret struct */
  static final short PG_RESPONSE_EVENT = 3; /* response is a pgresponse_event struct */
  static final short PG_RESPONSE_DATA  = 4; /* response is a pgresponse_data struct */

  /* the size to read from the server for the response packet */
  static final short SIZE = 12; /* short type + 5 * short info. */

  /**
   * return the size of the data we want to get
   */
  static public int size () {

    return SIZE;

  }

  /**
   * Construct with the given data
   */
  public _pgResponse (byte data[]) {

    type  = (short)(0x100 * (data[ 0] & 0xFF) + (data[ 1] & 0xFF));
    info1 = (short)(0x100 * (data[ 2] & 0xFF) + (data[ 3] & 0xFF));
    info2 = (short)(0x100 * (data[ 4] & 0xFF) + (data[ 5] & 0xFF));
    info3 = (short)(0x100 * (data[ 6] & 0xFF) + (data[ 7] & 0xFF));
    info4 = (short)(0x100 * (data[ 8] & 0xFF) + (data[ 9] & 0xFF));
    info5 = (short)(0x100 * (data[10] & 0xFF) + (data[11] & 0xFF));

  }
}

/**
 * _pgResponseErr : class to handle all the error response from pgserver
 */
public class _pgResponseErr {

  short errt;     /* error cathegory, one of the PG_ERRT_ constant */
  short msglen;   /* length in bytes of the associated text error message
		     following immediately the pgresponse_err */
  int id;         /* id of the request causing the error */

  static final short PG_ERRT_NONE     = 0x0000; /* No error condition */
  static final short PG_ERRT_MEMORY   = 0x0101; /*  Error allocating memory */
  static final short PG_ERRT_IO       = 0x0200; /*  Filesystem, operating system, or other IO error */
  static final short PG_ERRT_NETWORK  = 0x0300; /*  Network (or IPC) communication error */
  static final short PG_ERRT_BADPARAM = 0x0400; /*  Invalid parameters */
  static final short PG_ERRT_HANDLE   = 0x0500; /*  Invalid handle ID, type, or ownership */
  static final short PG_ERRT_INTERNAL = 0x0600; /*  Shouldn't happen (tell a developer!) */
  static final short PG_ERRT_BUSY     = 0x0700; /*  Try again later? */
  static final short PG_ERRT_FILEFMT  = 0x0800; /*  Error in a loaded file format (theme files, bitmaps) */
  static final short PG_ERRT_CLIENT   = (short)0x8000; /*  An error caused by the client lib, not the server */

  /**
   * Construct with the given response
   */
  public _pgResponseErr (_pgResponse rep) {

    errt   = rep.info1;
    msglen = rep.info2;
    id     = 0x10000 * rep.info4 + rep.info5;

  }
}

/**
 * _pgResponseRet : class to handle all the response of pgserver returning an int
 */
public class _pgResponseRet {

  int id;       /* id of the request causing the error */
  int data;     /* the value returned by pgserver for the given request */

  /* the size to read from the server for the response packet */
  static final short SIZE = 8; /* int id + int data */

  /**
   * return the size of the data we want to get
   */
  static public int size () {

    return SIZE;

  }

  /**
   * Construct with the given response
   */
  public _pgResponseRet (_pgResponse rep) {

    id   = 0x10000 * rep.info2 + rep.info3;
    data = 0x10000 * rep.info4 + rep.info5;

  }
}

/**
 * _pgResponseEvent : class to handle all the event received from pgserver
 */
public class _pgResponseEvent {

  short type;   /* type of the response. In this case PG_RESPONSE_EVENT */
  short event;  /* the event, one of the PG_WE_* or PG_NWE_* constant */
  int from;     /* the widget causing the event if PG_WE_*, of 0 if PG_NWE_* */
  int param;    /* packed data for the event */

  static final short PG_WE_ACTIVATE      = 0x001; /* Button has been clicked/selected */
  static final short PG_WE_DEACTIVATE    = 0x002; /* Sent when the user clicks outside the active popup */
  static final short PG_WE_CLOSE         = 0x003; /* A top-level widget has closed */
  static final short PG_WE_FOCUS         = 0x004; /* Sent when a button is focused, only if it has PG_EXEV_FOCUS. 
						     The field widget always sends this. */
  static final short PG_WE_PNTR_DOWN     = 0x204; /* The "mouse" button is now down */
  static final short PG_WE_PNTR_UP       = 0x205; /* The "mouse" button is now up */
  static final short PG_WE_PNTR_RELEASE  = 0x206; /* The "mouse" button was released outside the widget */
  static final short PG_WE_DATA          = 0x306; /* Widget is streaming data to the app */
  static final short PG_WE_RESIZE        = 0x107; /* For terminal widgets */
  static final short PG_WE_BUILD         = 0x108; /* Sent from a canvas, clients can rebuild groplist */
  static final short PG_WE_PNTR_MOVE     = 0x209; /* The "mouse" moved */
  static final short PG_WE_KBD_CHAR      = 0x40A; /* A focused keyboard character recieved */
  static final short PG_WE_KBD_KEYUP     = 0x40B; /* A focused raw keyup event */
  static final short PG_WE_KBD_KEYDOWN   = 0x40C; /* A focused raw keydown event */
  static final short PG_WE_APPMSG        = 0x301; /* Messages from another application */

  static final short PG_NWE_KBD_CHAR     = 0x140A; /* sent if the client has captured the keyboard */
  static final short PG_NWE_KBD_KEYUP    = 0x140B; /* sent if the client has captured the keyboard */
  static final short PG_NWE_KBD_KEYDOWN  = 0x140C; /* sent if the client has captured the keyboard */
  static final short PG_NWE_PNTR_MOVE    = 0x1209; /* sent if the client has captured the pointing device */
  static final short PG_NWE_PNTR_UP      = 0x1205; /* sent if the client has captured the pointing device */
  static final short PG_NWE_PNTR_DOWN    = 0x1204; /* sent if the client has captured the pointing device */
  static final short PG_NWE_BGCLICK      = 0x120D; /* The user clicked the background widget */
  static final short PG_NWE_PNTR_RAW     = 0x1101; /* Raw coordinates, for tpcal or games */
  static final short PG_NWE_CALIB_PENPOS = 0x1301; /* Raw 32-bit coordinates, for tpcal */

}

/**
 * _pgResponseData : class to handle all the response of pgserver returning more than an int
 */
public class _pgResponseData {

  short type;   /* type of the response. In this case PG_RESPONSE_RET */
  int id;       /* id of the request causing the error */
  int size;     /* the number of bytes of data following the pgresponse_data structure */

}

/**
 * _pgUtils class : misc utilities needed by the org.picogui classes
 */
public class _pgUtils {

    /* flag to control the display of the warning. Default to yes */
    static protected boolean _displayWarning = true;

  /* convert from a byte array to a String */
  static String _toString (byte b[]) {

    String str = new String ("");

    for( short i = 0; i < b.length; i++ ) {

      /* convert all the byte to chars, and append to the final string */
      str = str + (char)b[i];

    }

    /* return the final string */
    return str;

  }

    /* display standard messages */
    static void _client_msg (String txt) {

	VmShell.println( txt );

    }

    /* display error messages */
    static void _client_err (String txt) {

	VmShell.println( txt );

    }

    /* disable the warning display */
    static void _disableWarning () {

	_displayWarning = false;

    }

    /* enable the warning display */
    static void _enableWarning () {

	_displayWarning = true;

    }

    /* display warning messages */
    static void _client_warn (String txt) {

	if (_displayWarning) {

	    VmShell.println( txt );

	}
    }

    /* Receive data from the pgserver. Internal use only */
    /* return true if all is OK, false else */ 
    static boolean _pg_recv (Socket _pg_socket, byte data[], int datasize) {

	int count = _pg_socket.readBytes (data, 0, datasize);
	if( count < 0 ) {

	    _client_err ("recv error");
	    return false;

	}

	return true;

    }

  /* Send data to the server, checking and reporting errors */
  static boolean _pg_send (Socket _pg_socket, byte data[], int datasize) {

    if (_pg_socket.writeBytes (data, 0, datasize) != datasize) {

      _client_err ("send error");
      return false;

    }
    return true;
  }

}

/**
 * NetCore class : the base clase which connect to the pgserver
 */
public class NetCore {

    /* the default port */
    static final int PG_REQUEST_PORT = 30450;

    /* the default server */
    static final String PG_REQUEST_SERVER = "127.0.0.1";

    /* the socket to the pgserver */
    private Socket _pg_socket = null;

  /* if getAnswer () get some data, it will put it here */
  int _pg_answer_data;

  /* the request id counter */
  private int _pg_request_id = 0;

    /**
     * Constructor
     */
    public NetCore () {
    }

  /**
   * send the given request to the server
   */
  private boolean _pgSendReq (_pgRequest req) {

    return _pgUtils._pg_send (_pg_socket, req.inBytes (), req.size ());

  }

  /**
   * get the answer from the server
   */
  private boolean _pgGetAnswer (int theID) {

    /* read the answer for this request */
    byte data[]  = new byte[ _pgResponse.size () ];
    if ( !_pgUtils._pg_recv (_pg_socket, data, _pgResponse.size ())) {

      /* problem reading the type of response */
      _pgUtils._client_err ("error reading type of answer");
      return false;

    }

    /* create now the response with the given data */
    _pgResponse rep = new _pgResponse (data);

    /* processing depending of the response type */
    switch (rep.type) {

    case _pgResponse.PG_RESPONSE_ERR:

      /* an error has occured */

      /* convert the response to the correct type */
      _pgResponseErr rep_err = new _pgResponseErr (rep);

      if (theID != rep_err.id) {

	/* not the correct ID ! */
	_pgUtils._client_err ("out of sync (id answer is not what we expect)");
	return false;

      }

      /* get the message from the server */
      byte msg[]  = new byte[ rep_err.msglen ];
      if ( !_pgUtils._pg_recv (_pg_socket, msg, rep_err.msglen)) {

	/* problem reading the type of response */
	_pgUtils._client_err ("error reading error message");
	return false;

      }

      /* inform the user */
      _pgUtils._client_err ("error response received :" + _pgUtils._toString (msg) + ".");
      return false;

    case _pgResponse.PG_RESPONSE_RET:

      /* more common response : one int is returned */

      /* convert the response to the correct type */
      _pgResponseRet rep_ret = new _pgResponseRet (rep);

      if (theID != rep_ret.id) {

	/* not the correct ID ! */
	_pgUtils._client_err ("out of sync (id answer is not what we expect)");
	return false;

      }

      /* all is OK => put the result in the object variable */
      _pg_answer_data = rep_ret.data;

      break;

    case _pgResponse.PG_RESPONSE_EVENT:
      break;

    case _pgResponse.PG_RESPONSE_DATA:
      break;

    default: /* not a correct response type */
      _pgUtils._client_err ("wrong response type");
      return false;

    }

    /* all was OK */
    return true;

  }

    /**
     * Initialize PicoGUI
     * return true if all is OK, false else
     */
    public boolean pgInit (String[] args) {

	/* where is the pgserver ? */
	String server = PG_REQUEST_SERVER;
	int port = PG_REQUEST_PORT;

	/* handle the given args we recognize, leave other for the application */
	for (short i = 0; (i < args.length) && (args[i] != null); i++) {

	    /* by default : remove one parameter */
	    short args_to_shift = 1;

	    if (args[i].substring (0, 4).equals ("--pg")) {

		String argument = args[i].substring (4,args[i].length ());

		if (argument.equals ("server")) {

		    /* --pgserver : Next argument is the picogui server */
		    args_to_shift = 2;
		    server = args[i+1];

		    /* skip the server name */
		    i++;

		} else if (argument.equals ("version")) {

		    /* --pgversion : For now print CVS id */
		    _pgUtils._client_err ("$Id$");

		    return false;

		} else if (argument.equals ("applet")) {

		    /* --pgapplet : Create the app in a public container instead of
		     *              registering a new app.
		     */

		    _pgUtils._client_err ("Not implemented now. Sorry.");

		    return false;

		} else if (argument.equals ("nowarn")) {

		    /* disable warning display */
		    _pgUtils._disableWarning ();

		} else {

		    /* not a correct --pg argument => display a small help and quit */
		    _pgUtils._client_err ( "PicoGUI Client Library\n"
					   + "Commands: --pgserver server:display --pgversion --pgapplet --pgnowarn\n");
		    return false;

		}
	    }
	}

	_pgUtils._client_warn ("TODO: Need to handle args");

	/* Separate the display number from the hostname */
	if( server.indexOf (':') >= 0 ) {

	    /* we have a display specification */

	    /* get the display number */
	    port = PG_REQUEST_PORT + Convert.toInt( server.substring (server.indexOf (':') + 1, server.length ()));

	    if( server.indexOf (':') > 0 ) {

	      /* we have a server name => get it */
	      server = server.substring (0, server.indexOf (':'));

	    } else {

	      /* no server given => take the default */
	      server = PG_REQUEST_SERVER;

	    }
	}

	/* connect to the wanted server */
	_pg_socket = new Socket (server, port);

	if (_pg_socket.isOpen()) {

	    /* OK, we are connected to a server */

	    /* read the ident of the server */
	    byte data[] = new byte[ _pgHello.SIZE ];
	    if (_pgUtils._pg_recv (_pg_socket, data, _pgHello.SIZE)) {

		/* OK, we have read the ident */
		_pgHello ServerInfo = new _pgHello (data);

	    } else {

		/* the ident can't be read */
		_pgUtils._client_err ("can't read ident");

		/* close the socket and exit */
		_pg_socket.close ();
		return false;

	    }

	} else {

	    /* the connection to the server fail */
	    _pgUtils._client_err ("Error connecting to server " + server + " on port " + port);
	    return false;

	}

	/* all was OK */
	return true;

    }

  /**
   * "ping" the pgserver
   */
  public boolean pgPing() {

    /* prepare the request to send */
    _pgRequest req = new _pgRequest (_pg_request_id, _pgRequest.PGREQ_PING);

    /* send the request */
    if (!_pgSendReq (req)) {

      /* problem sending the request */
      _pgUtils._client_err ("error sending request");
      return false;

    }

    /* read the answer */
    if (!_pgGetAnswer (_pg_request_id)) {

      /* problem reading the answer */
      _pgUtils._client_err ("error getting answer");
      return false;

    }

    /* update the request id counter */
    _pg_request_id++;

    /* all was OK */
    return true;

  }

  /**
   * ask pgserver to update the screen
   */
  public boolean pgUpdate () {

    _pgRequest req = new _pgRequest (_pg_request_id, _pgRequest.PGREQ_UPDATE);

    /* send the request */
    if (!_pgSendReq (req)) {

      /* problem sending the request */
      _pgUtils._client_err ("error sending request");
      return false;

    }

    /* read the answer */
    if (!_pgGetAnswer (_pg_request_id)) {

      /* problem reading the answer */
      _pgUtils._client_err ("error getting answer");
      return false;

    }

    /* update the request id counter */
    _pg_request_id++;

    /* all was OK */
    return true;

  }

  /**
   * creation of a new string with an array of bytes
   */
  public int pgNewString (byte b[]) {

    _pgRequest req = new _pgRequest (_pg_request_id, _pgRequest.PGREQ_MKSTRING, b);

    /* send the request */
    if (!_pgSendReq (req)) {

      /* problem sending the request */
      _pgUtils._client_err ("error sending request");
      return 0;

    }

    /* read the answer */

    if (!_pgGetAnswer (_pg_request_id)) {

      /* problem reading the answer */
      _pgUtils._client_err ("error getting answer");
      return 0;

    }

    /* update the request id counter */
    _pg_request_id++;

    /* all was OK */
    return _pg_answer_data;

  }

  /**
   * creation of a new string with an String
   */
  public int pgNewString (String s) {
    byte b[] = new byte [s.length() + 1];

    for( int i = 0; i < s.length(); i++ ) {

      /* extract the current char, convert it to a byte and put it in the byte array */
      b[i] = (byte)s.charAt (i);

    }

    /* OK, now we have a byte array, so we can call the other method */
    return pgNewString (b);

  }

  /**
   * register the application
   */
  public int pgRegisterApp (String s, short type) {

    int theName = pgNewString (s);

    _pgReqdRegister reqd = new _pgReqdRegister (theName, type);

    _pgRequest req = new _pgRequest (_pg_request_id, _pgRequest.PGREQ_REGISTER, reqd.inBytes ());

    /* send the request */
    if (!_pgSendReq (req)) {

      /* problem sending the request */
      _pgUtils._client_err ("error sending request");
      return 0;

    }

    /* read the answer */

    if (!_pgGetAnswer (_pg_request_id)) {

      /* problem reading the answer */
      _pgUtils._client_err ("error getting answer");
      return 0;

    }

    /* update the request id counter */
    _pg_request_id++;

    /* all was OK */
    return _pg_answer_data;

  }

}
