/* flags */
#define AUTOCONNECT				1
#define USE_SSL					2
#define ACCEPT_INVALID_CERT	4
#define DONT_USE_PROXY			8

struct slentry
{
	/* Port: expanded state for groups, port for servers */
	int port;
	char channel[CHANLEN];
	char server[132];
	char password[86];
	char comment[100];
	char nick[NICKLEN];
	char eom_cmd[72];	/* end of motd command */
	int flags;
};

struct defaultserv
{
	char *channel;
	char *server;
	char *comment;
	int port;
};

static struct defaultserv dserv[] =
{
        /* Debian servers. */
        {"", "SUB", "OpenProjects Network",1},
        {"#debian", "irc.debian.org", "Debian Support Channel", 6667},
        {"#debian", "irc.openprojects.net", "Debian Support Channel", 6667},
        {"", "ENDSUB", "", 0},

        {"", "SUB", "ChatJunkiesNet", 0},	
	{"#linux", "irc.xchat.org", "ChatJunkiesNet Random Server #linux", 6667},
	{"#linux", "us.xchat.org", "ChatJunkiesNet U.S. Server #linux", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "IRCNet", 0},
	{"", "us.ircnet.org", "IRCNet USA", 6667},
	{"", "eu.ircnet.org", "IRCNet Europe", 6667},
	{"", "irc.ircd.it", "IRCNet Italy", 6667},
	{"", "au.ircnet.org", "IRCNet Australia", 6667},
	{"", "irc.stealth.net", "irc.stealth.net", 6660},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "DALNet", 0},
	{"", "irc.dal.net", "DALNet USA", 6667},
	{"", "irc.eu.dal.net", "DALNet Europe", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "EFNet", 0},
	{"", "us.rr.efnet.net", "EFNet USA", 6667},
	{"", "ca.rr.efnet.net", "EFNet Canada", 6667},
	{"", "eu.rr.efnet.net", "EFNet Europe", 6667},
	{"", "au.rr.efnet.net", "EFNet Australia", 6667},
	{"", "irc.efnet.org", "irc.efnet.org", 6667},
	{"", "irc.light.se", "irc.light.se", 6667},
	{"", "irc.stanford.edu", "irc.stanford.edu", 6667},
	{"", "irc.solidstreaming.net", "irc.solidstreaming.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "GalaxyNet", 0},
	{"", "sprynet.us.galaxynet.org", "sprynet.us.galaxynet.org", 6667},
	{"", "atlanta.ga.us.galaxynet.org", "atlanta.ga.us.galaxynet.org", 6667},
	{"", "irc.galaxynet.org", "irc.galaxynet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "UnderNet", 0},
	{"", "us.undernet.org", "UnderNet USA", 6667},
	{"", "eu.undernet.org", "UnderNet Europe", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "OpenProjectsNet", 0},
	{"", "irc.openprojects.net", "irc.openprojects.net", 6667},
	{"", "irc.linux.com", "irc.linux.com", 6667},
	{"", "us.openprojects.net", "us.openprojects.net", 6667},
	{"", "eu.openprojects.net", "eu.openprojects.net", 6667},
	{"", "au.openprojects.net", "au.openprojects.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "OtherNet", 0},
	{"", "irc.othernet.org", "irc.othernet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AustNet", 0},
	{"", "us.austnet.org", "AustNet US", 6667},
	{"", "ca.austnet.org", "AustNet CA", 6667},
	{"", "au.austnet.org", "AustNet AU", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "OzNet", 0},
	{"", "sydney.oz.org", "sydney.oz.org", 6667},
	{"", "melbourne.oz.org", "melbourne.oz.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "FoxChat", 0},
	{"", "irc.FoxChat.net", "irc.FoxChat.net", 6667},
	{"", "irc.ac6.org", "irc.ac6.org", 6667},
	{"", "beastie.ac6.org", "beastie.ac6.org", 6667},
	{"", "wild.FoxChat.net", "wild.FoxChat.net", 6667},
	{"", "roadkill.FoxChat.net", "roadkill.FoxChat.net", 6667},
	{"", "slick.FoxChat.net", "slick.FoxChat.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "IrcLink", 0},
	{"", "irc.irclink.net", "irc.irclink.net", 6667},
	{"", "Alesund.no.eu.irclink.net", "Alesund.no.eu.irclink.net", 6667},
	{"", "Oslo.no.eu.irclink.net", "Oslo.no.eu.irclink.net", 6667},
	{"", "frogn.no.eu.irclink.net", "frogn.no.eu.irclink.net", 6667},
	{"", "tonsberg.no.eu.irclink.net", "tonsberg.no.eu.irclink.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "XWorld", 0},
	{"#chatzone", "Buffalo.NY.US.XWorld.org", "Buffalo.NY.US.XWorld.org", 6667},
	{"#chatzone", "Minneapolis.MN.US.Xworld.Org", "Minneapolis.MN.US.Xworld.Org", 6667},
	{"#chatzone", "PalmSprings.CA.US.XWorld.Org", "PalmSprings.CA.US.XWorld.Org", 6667},
	{"#chatzone", "Quebec.QC.CA.XWorld.Org", "Quebec.QC.CA.XWorld.Org", 6667},
	{"#chatzone", "Rochester.NY.US.XWorld.org", "Rochester.NY.US.XWorld.org", 6667},
	{"#chatzone", "Bayern.DE.EU.XWorld.Org", "Bayern.DE.EU.XWorld.Org", 6667},
	{"#chatzone", "Chicago.IL.US.XWorld.Org", "Chicago.IL.US.XWorld.Org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "ChatNet", 0},
	{"", "US.ChatNet.Org", "ChatNet USA", 6667},
	{"", "EU.ChatNet.Org", "ChatNet Europe", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AnyNet", 0},
	{"#anynet", "irc.anynet.org", "irc.anynet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "KewlNet", 0},
	{"", "irc.kewl.org", "irc.kewl.org", 6667},
	{"", "la.defense.fr.eu.kewl.org", "la.defense.fr.eu.kewl.org", 6667},
	{"", "nanterre.fr.eu.kewl.org", "nanterre.fr.eu.kewl.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "MagicStar", 0},
	{"", "irc.magicstar.net", "irc.magicstar.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "SceneNet", 0},
	{"", "irc.scene.org", "irc.scene.org", 6667},
	{"", "irc.eu.scene.org", "irc.eu.scene.org", 6667},
	{"", "irc.us.scene.org", "irc.us.scene.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "StarChat", 0},
	{"", "irc.starchat.net", "Random StarChat Server", 6667},
	{"", "galatea.starchat.net", "Washington, US", 6667},
	{"", "stargate.starchat.net", "Florida, US", 6667},
	{"", "powerzone.starchat.net", "Texas, US", 6667},
	{"", "utopia.starchat.net", "United Kingdom, EU", 6667},
	{"", "cairns.starchat.net", "Queensland, AU", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "Infinity-IRC.org", 0},
	{"#Linux", "Atlanta.GA.US.Infinity-IRC.Org", "Atlanta GA #Linux", 6667},
	{"#Linux", "Babylon.NY.US.Infinity-IRC.Org", "New York, NY #Linux", 6667},
	{"#Linux", "Dewspeak.TX.US.Infinity-IRC.Org", "San Antonio TX #Linux", 6667},
	{"#Linux", "Sunshine.Ca.US.Infinity-IRC.Org", "San Jose CA #Linux", 6667},
	{"#Linux", "MNC.MD.Infinity-IRC.Org", "Moldava, former Soviet Union #Linux", 6667},
	{"#Linux", "IRC.Infinity-IRC.Org", "Random Server #Linux", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "EUIrc", 0},
	{"", "irc.euirc.net", "irc.euirc.net", 6667},
	{"", "irc.ham.de.euirc.net", "irc.ham.de.euirc.net", 6667},
	{"", "irc.ber.de.euirc.net", "irc.ber.de.euirc.net", 6667},
	{"", "irc.ffm.de.euirc.net", "irc.ffm.de.euirc.net", 6667},
	{"", "irc.bre.de.euirc.net", "irc.bre.de.euirc.net", 6667},
	{"", "irc.hes.de.euirc.net", "irc.hes.de.euirc.net", 6667},
	{"", "irc.vie.at.euirc.net", "irc.vie.at.euirc.net", 6667},
	{"", "irc.inn.at.euirc.net", "irc.inn.at.euirc.net", 6667},
	{"", "irc.bas.ch.euirc.net", "irc.bas.ch.euirc.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "HabberNet", 0},
	{"", "irc.habber.net", "Random HabberNet Server", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "Mellorien", 0},
	{"", "Irc.mellorien.net", "Irc.mellorien.net", 6667},
	{"", "us.mellorien.net", "us.mellorien.net", 6667},
	{"", "eu.mellorien.net", "eu.mellorien.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "FEFNet", 0},
	{"", "irc.fef.net", "irc.fef.net", 6667},
	{"", "irc.villagenet.com", "irc.villagenet.com", 6667},
	{"", "irc.ggn.net", "irc.ggn.net", 6667},
	{"", "irc.vendetta.com", "irc.vendetta.com", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "Gamma Force", 0},
	{"", "irc.gammaforce.org", "Random Gamma Force server", 6667},
	{"", "sphinx.or.us.gammaforce.org", "Sphinx: US, Orgeon", 6667},
	{"", "monolith.ok.us.gammaforce.org", "Monolith: US, Oklahoma", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "KrushNet.Org", 0},
	{"", "Jeffersonville.IN.US.KrushNet.Org", "Jeffersonville.IN.US.KrushNet.Org", 6667},
	{"", "Auckland.NZ.KrushNet.Org", "Auckland.NZ.KrushNet.Org", 6667},
	{"", "Hastings.NZ.KrushNet.Org", "Hastings.NZ.KrushNet.Org", 6667},
	{"", "Seattle-R.WA.US.KrushNet.Org", "Seattle-R.WA.US.KrushNet.Org", 6667},
	{"", "Minneapolis.MN.US.KrushNet.Org", "Minneapolis.MN.US.KrushNet.Org", 6667},
	{"", "Cullowhee.NC.US.KrushNet.Org", "Cullowhee.NC.US.KrushNet.Org", 6667},
	{"", "Asheville-R.NC.US.KrushNet.Org", "Asheville-R.NC.US.KrushNet.Org", 6667},
	{"", "San-Antonio.TX.US.KrushNet.Org", "San-Antonio.TX.US.KrushNet.Org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "Neohorizon", 0},
	{"", "irc.nhn.net", "irc.nhn.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "TrekLink", 0},
	{"", "neutron.treklink.net", "neutron.treklink.net", 6667},
	{"", "irc.treklink.net", "irc.treklink.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "Librenet", 0},
	{"", "irc.librenet.net", "irc.librenet.net", 6667},
	{"", "famipow.fr.librenet.net", "famipow.fr.librenet.net", 6667},
	{"", "ielf.fr.librenet.net", "ielf.fr.librenet.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "PTNet, UNI", 0},
	{"", "irc.PTNet.org", "irc.PTNet.org", 6667},
	{"", "rccn.PTnet.org", "rccn.PTnet.org", 6667},
	{"", "uevora.PTnet.org", "uevora.PTnet.org", 6667},
	{"", "umoderna.PTnet.org", "umoderna.PTnet.org", 6667},
	{"", "ist.PTnet.org", "ist.PTnet.org", 6667},
	{"", "aaum.PTnet.org", "aaum.PTnet.org", 6667},
	{"", "uc.PTnet.org", "uc.PTnet.org", 6667},
	{"", "ualg.ptnet.org", "ualg.ptnet.org", 6667},
	{"", "madinfo.PTnet.org", "madinfo.PTnet.org", 6667},
	{"", "isep.PTnet.org", "isep.PTnet.org", 6667},
	{"", "ua.PTnet.org", "ua.PTnet.org", 6667},
	{"", "ipg.PTnet.org", "ipg.PTnet.org", 6667},
	{"", "isec.PTnet.org", "isec.PTnet.org", 6667},
	{"", "utad.PTnet.org", "utad.PTnet.org", 6667},
	{"", "iscte.PTnet.org", "iscte.PTnet.org", 6667},
	{"", "ubi.PTnet.org", "ubi.PTnet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "PTNet, ISP's", 0},
	{"", "irc.PTNet.org", "irc.PTNet.org", 6667},
	{"", "rccn.PTnet.org", "rccn.PTnet.org", 6667},
	{"", "EUnet.PTnet.org", "EUnet.PTnet.org", 6667},
	{"", "madinfo.PTnet.org", "madinfo.PTnet.org", 6667},
	{"", "netc2.PTnet.org", "netc2.PTnet.org", 6667},
	{"", "netc1.PTnet.org", "netc1.PTnet.org", 6667},
	{"", "teleweb.PTnet.org", "teleweb.PTnet.org", 6667},
	{"", "netway.PTnet.org", "netway.PTnet.org", 6667},
	{"", "telepac1.ptnet.org", "telepac1.ptnet.org", 6667},
	{"", "services.ptnet.org", "services.ptnet.org", 6667},
	{"", "esoterica.PTnet.org", "esoterica.PTnet.org", 6667},
	{"", "ip-hub.ptnet.org", "ip-hub.ptnet.org", 6667},
	{"", "telepac1.ptnet.org", "telepac1.ptnet.org", 6667},
	{"", "nortenet.PTnet.org", "nortenet.PTnet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "ARCNet", 0},
	{"", "se1.arcnet.vapor.com", "se1.arcnet.vapor.com", 6667},
	{"", "us1.arcnet.vapor.com", "us1.arcnet.vapor.com", 6667},
	{"", "us2.arcnet.vapor.com", "us2.arcnet.vapor.com", 6667},
	{"", "us3.arcnet.vapor.com", "us3.arcnet.vapor.com", 6667},
	{"", "ca1.arcnet.vapor.com", "ca1.arcnet.vapor.com", 6667},
	{"", "de1.arcnet.vapor.com", "de1.arcnet.vapor.com", 6667},
	{"", "de3.arcnet.vapor.com", "de3.arcnet.vapor.com", 6667},
	{"", "ch1.arcnet.vapor.com", "ch1.arcnet.vapor.com", 6667},
	{"", "be1.arcnet.vapor.com", "be1.arcnet.vapor.com", 6667},
	{"", "nl3.arcnet.vapor.com", "nl3.arcnet.vapor.com", 6667},
	{"", "uk1.arcnet.vapor.com", "uk1.arcnet.vapor.com", 6667},
	{"", "uk2.arcnet.vapor.com", "uk2.arcnet.vapor.com", 6667},
	{"", "uk3.arcnet.vapor.com", "uk3.arcnet.vapor.com", 6667},
	{"", "fr1.arcnet.vapor.com", "fr1.arcnet.vapor.com", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "NeverNET", 0},
	{"", "irc.nevernet.net", "irc.nevernet.net", 6667},
	{"", "imagine.nevernet.net", "imagine.nevernet.net", 6667},
	{"", "dimension.nevernet.net", "dimension.nevernet.net", 6667},
	{"", "universe.nevernet.net", "universe.nevernet.net", 6667},
	{"", "wayland.nevernet.net", "wayland.nevernet.net", 6667},
	{"", "forte.nevernet.net", "forte.nevernet.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "PTlink", 0},
	{"", "irc.PTlink.net", "irc.PTlink.net", 6667},
	{"", "dark.PTlink.net", "dark.PTlink.net", 6667},
	{"", "uc.PTlink.net", "uc.PTlink.net", 6667},
	{"", "kungfoo.PTlink.net", "kungfoo.PTlink.net", 6667},
	{"", "matrix.PTlink.net", "matrix.PTlink.net", 6667},
	{"", "illusion.PTlink.net", "illusion.PTlink.net", 6667},
	{"", "Cibercultura.PTlink.net", "Cibercultura.PTlink.net", 6667},
	{"", "aaia.PTlink.net", "aaia.PTlink.net", 6667},
	{"", "gaesi.PTlink.net", "gaesi.PTlink.net", 6667},
	{"", "BuBix.PTlink.net", "BuBix.PTlink.net", 6667},
	{"", "montijo.PTlink.net", "montijo.PTlink.net", 6667},
	{"", "queima.PTlink.net", "queima.PTlink.net", 6667}, 
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AstroLINK.Org", 0},
	{"", "irc.astrolink.org", "irc.astrolink.org", 6667}, 
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AxeNet", 0},
	{"", "irc.axenet.org", "irc.axenet.org", 6667},
	{"", "angel.axenet.org", "angel.axenet.org", 6667},
	{"", "energy.axenet.org", "energy.axenet.org", 6667},
	{"", "python.axenet.org", "python.axenet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "DwarfStarNet", 0},
	{"", "IRC.dwarfstar.net", "IRC.dwarfstar.net", 6667},
	{"", "US.dwarfstar.net", "US.dwarfstar.net", 6667},
	{"", "EU.dwarfstar.net", "EU.dwarfstar.net", 6667},
	{"", "AU.dwarfstar.net", "AU.dwarfstar.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "SlashNET", 0},
	{"", "irc.slashnet.org", "irc.slashnet.org", 6667},
	{"", "area51.slashnet.org", "area51.slashnet.org", 6667},
	{"", "moo.slashnet.org", "moo.slashnet.org", 6667},
	{"", "radon.slashnet.org", "radon.slashnet.org", 6667},
	{"", "devnull.slashnet.org", "devnull.slashnet.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AzzurraNet", 0},
	{"", "irc.bitchx.it", "irc.bitchx.it", 6667},
	{"", "irc.jnet.it", "irc.jnet.it", 6667},
	{"", "irc.net36.com", "irc.net36.com", 6667},
	{"", "irc.noflyzone.net", "irc.noflyzone.net", 6667},
	{"", "irc.swappoint.com", "irc.swappoint.com", 6667},
	{"", "irc.azzurra.com", "irc.azzurra.com", 6667},
	{"", "irc.leonet.it", "irc.leonet.it", 6667},
	{"", "irc.libero.it", "irc.libero.it", 6667},
	{"", "irc.estranet.it", "irc.estranet.it", 6667},  
	{"", "irc.filmaker.it", "irc.filmaker.st", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "NixHelpNet", 0},
	{"", "irc.nixhelp.org", "irc.nixhelp.org", 6667},
	{"", "us.nixhelp.org", "Random US NixHelp Server", 6667},
	{"", "uk.nixhelp.org", "Random UK NixHelp Server", 6667},
	{"", "uk2.nixhelp.org", "UK2 NixHelp Server", 6667},
	{"", "uk3.nixhelp.org", "UK3 NixHelp Server", 6667},
	{"", "nl.nixhelp.org", "Netherlands NixHelp Server", 6667},
	{"", "ca.ld.nixhelp.org", "Canada NixHelp Server", 6667},
	{"", "us.co.nixhelp.org", "US Colorado NixHelp Server", 6667},
	{"", "us.ca.nixhelp.org", "US California NixHelp Server", 6667},
	{"", "us.pa.nixhelp.org", "US Pennsylvania NixHelp Server", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "RebelChat", 0},
	{"", "irc.rebelchat.org", "RebelChat Random Server", 6667},
	{"", "interquad.rebelchat.org", "interquad.rebelchat.org", 6667},
	{"", "rebel.rebelchat.org", "rebel.rebelchat.org", 6667},
	{"", "bigcove.rebelchat.org", "bigcove.rebelchat.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "DragonLynk", 0},
	{"", "irc.dragonlynk.net", "DragonLynk", 6667},
	{"", "ENDSUB", "", 0},

/* down?
	{"", "SUB", "Lunarnet IRC", 0},
	{"", "isarania.lunarnetirc.org", "isarania.lunarnetirc.org", 6667},
	{"", "ayerie.lunarnetirc.org", "ayerie.lunarnetirc.org", 6667},
	{"", "ENDSUB", "", 0},*/

	{"", "SUB", "CoolChat", 0},
	{"", "irc.coolchat.net", "CoolChat: Random Server", 6667},
	{"", "unix.coolchat.net", "CoolChat: Los Angeles, CA, US", 6667},
	{"", "south.coolchat.net", "CoolChat: Atlanta, GA, US", 6667},
	{"", "toronto.coolchat.net", "CoolChat: Toronto, ON, CA", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "unsecurity.org", 0},
	{"", "irc.unsecurity.org", "irc.unsecurity.org", 6667},
	{"", "wc.unsecurity.org", "wc.unsecurity.org", 6667},
	{"", "thegift.unsecurity.org", "thegift.unsecurity.org", 6667},
	{"", "sysgate.unsecurity.org", "sysgate.unsecurity.org", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "QChat.net", 0},
	{"", "irc.qchat.net", "irc.qchat.net", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "SubCultNet", 0},
	{"", "irc.subcult.ch", "irc.subcult.ch", 6667},
	{"", "irc.phuncrew.ch", "irc.phuncrew.ch", 6668},
	{"", "irc.mgz.ch", "irc.mgz.ch", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AbleNET", 0},
	{"", "california.ablenet.org", "AbleNET California", 6667},
	{"", "amazon.ablenet.org", "AbleNET Tulsa, OK", 6667},
	{"", "agora.ablenet.org", "AbleNET Portland, Oregon", 6667},
	{"", "extreme.ablenet.org", "AbleNET Texus", 6667},
	{"", "irc.ablenet.org", "AbleNET Random Server", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "FDFNet", 0},
	{"", "irc.fdfnet.net", "FDFNet - US. Canada. Mexico", 6667},
	{"", "irc.eu.fdfnet.net", "FDFNet - Europe", 6667},
	{"", "ENDSUB", "", 0},

	{"", "SUB", "AfterNET", 0},
	{"", "irc.afternet.org", "AfterNET Random Server", 6667},
	{"", "ic5.eu.afternet.org", "Helsingborg, Sweden, EU", 6667},
	{"", "baltimore.md.us.afternet.org", "Baltimore, Maryland, USA", 6667},
	{"", "boston.afternet.org", "Boston, Massachusetts, USA", 6667},
	{"", "ENDSUB", "", 0},

	{0, 0, 0, 0}
};
