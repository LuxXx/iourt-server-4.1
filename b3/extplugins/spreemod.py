# b3/plugins/spreemod.py
#
# This plugin will show killing or loosing spree messages.
#
# It's written with fun, for fun. Please report any errors or
# if something can be done more efficient to Walker@1stsop.nl
#
# 08-03-2005, Walker: Changed the end spree messages a bit. 
# 08-01-2005, ThorN:  Code change suggestions 
# 08-01-2005, Walker: Initial creation 
#
# all plugins must import b3 and b3.events
import b3
import b3.events
import b3.plugin


class SpreeStats:
    kills                  = 0
    deaths                 = 0
    endLoosingSpreeMessage = None
    endKillSpreeMessage    = None
    
#--------------------------------------------------------------------------------------------------
class SpreemodPlugin(b3.plugin.Plugin):
    _adminPlugin = None
    _killingspree_messages_dict = {}
    _loosingspree_messages_dict = {}
    _winningspree_commands_dict = {}
    _loosingspree_commands_dict = {}
    _reset_spree_stats = 0
    _min_level_spree_cmd = 0
    _clientvar_name = 'spree_info'
    
    def startup(self):
        """\
        Initialize plugin settings
        """

        self.debug('Starting')
        # get the plugin so we can register commands
        self._adminPlugin = self.console.getPlugin('admin')
        if not self._adminPlugin:
            # something is wrong, can't start without admin plugin
            self.error('Could not find admin plugin')
            return False

        # Get the settings from the config.
        if __name__ == '__main__':
           self._reset_spree_stats = 1
           self._min_level_spree_cmd = 1
        else:
           if self.config.get('settings', 'reset_spree') == '1':
              self._min_level_spree_cmd = self.config.getint('settings', 'min_level_spree_cmd')
        
        # register our !spree command
        self.verbose('Registering commands')
        self._adminPlugin.registerCommand(self, 'spree', self._min_level_spree_cmd, self.cmd_spree)

        # listen for client events
        self.verbose('Registering events')
        self.registerEvent(b3.events.EVT_CLIENT_KILL)
        self.registerEvent(b3.events.EVT_GAME_EXIT)

        # Initialize the message list used in this plugin
        self.init_spreemessage_list()

        self.debug('Started')

    def onLoadConfig(self):
        self.init_spreemessage_list()

    def handle(self, event):
        """\
        Handle intercepted events
        """
        if event.type == b3.events.EVT_CLIENT_KILL:
             self.handle_kills(event.client, event.target)
        elif event.type == b3.events.EVT_GAME_EXIT:
             if self._reset_spree_stats:
                for c in self.console.clients.getList():
                   self.init_spree_stats(c)
              
    def init_spreemessage_list(self):
        # Get the spree messages from the config
        # Split the start and end spree messages and save it in the dictionary
        if __name__ == '__main__':
           self._killingspree_messages_dict[7] = ['5Kills %player%', '%victim% died by %player']
           self._killingspree_messages_dict[10] = ['10Kills %player%', '%victim% died by %player']
           self._killingspree_messages_dict[15] = ['15Kills %player%', '%victim% died by %player']
           self._loosingspree_messages_dict[7] = ['7deaths %player%', '%victim% died by %player']
           self._winningspree_commands_dict[5] = 'kick %player%'
        else:
           for kills, message  in self.config.items('killingspree_messages'):
            # force the kills to an integer
              self._killingspree_messages_dict[int(kills)]  = message.split('#')
        
           for deaths, message in self.config.items('loosingspree_messages'):
              self._loosingspree_messages_dict[int(deaths)] = message.split('#')
              
           for deaths, message in self.config.items('winningspree_commands'):
              self._winningspree_commands_dict[int(deaths)] = message;
           
           for deaths, message in self.config.items('loosingspree_commands'):
              self._loosingspree_commands_dict[int(deaths)] = message;
              
        self.verbose('spree-messages and commands are loaded in memory')

    def init_spree_stats(self, client):
        # initialize the clients spree stats
        client.setvar(self, self._clientvar_name, SpreeStats())
    
    def get_spree_stats(self, client):
        # get the clients stats
        # pass the plugin reference first
        # the key second
        # the defualt value first
        
        if not client.isvar(self, self._clientvar_name):
            # initialize the default spree object
            # we don't just use the client.var(...,default) here so we
            # don't create a new SpreeStats object for no reason every call
            client.setvar(self, self._clientvar_name, SpreeStats())
            
        return client.var(self, self._clientvar_name).value
    
    def handle_kills(self, client=None, victim=None):
        """\
        A kill was made. Add 1 to the client and set his deaths to 0.
        Add 1 death to the victim and set his kills to 0.
        """

        # client (attacker)
        if client:
            # we grab our SpreeStats object here
            # any changes to its values will be saved "automagically"
            spreeStats = self.get_spree_stats(client)
            spreeStats.kills += 1
            
            # Check if the client was on a loosing spree. If so then show his end loosing spree msg.
            if spreeStats.endLoosingSpreeMessage:
                self.show_message( client, victim, spreeStats.endLoosingSpreeMessage )
                # reset any possible loosing spree to None
                spreeStats.endLoosingSpreeMessage = None
            # Check if the client is on a killing spree. If so then show it.
            message = self.get_spree_message(spreeStats.kills, 0)
            if message:
                #Save the 'end'spree message in the client. That is used when the spree ends.
                spreeStats.endKillSpreeMessage = message[1]
                #Show the 'start'spree message
                self.show_message( client, victim, message[0])
            
            command = self._winningspree_commands_dict.get(spreeStats.kills, None)
            if command:
               command = command.replace('%player%', client.cid)
               self.console.write(command)

            # deaths spree is over, reset deaths
            spreeStats.deaths = 0

        # Victim
        if victim:
            spreeStats = self.get_spree_stats(victim)
            spreeStats.deaths += 1
            
            # Check if the victim had a killing spree and show a end_killing_spree message
            if spreeStats.endKillSpreeMessage:
                self.show_message( client, victim, spreeStats.endKillSpreeMessage )
                # reset any possible end spree to None
                spreeStats.endKillSpreeMessage = None

            #Check if the victim is on a 'loosing'spree
            message = self.get_spree_message(0, spreeStats.deaths)
            if message:
                #Save the 'loosing'spree message in the client.
                spreeStats.endLoosingSpreeMessage = message[1]
                
                self.show_message( victim, client, message[0] )
            
            command = self._loosingspree_commands_dict.get(spreeStats.deaths, None)
            if command:
               command = command.replace('%player%', client.name)
               self.console.write(command)
            
            # kill spree is over, reset kills
            spreeStats.kills = 0

    def get_spree_message(self, kills, deaths):
        
        # default is None, there is no message
        message = None
        
        # killing spree check
        if kills != 0:
            # if there is an entry for this number of kills, grab it, otherwise
            # return None
            
            # -----------------
            #if kills == 5:
            #	client.message("You got 5 kills!!")
            #elif kills == 10:
            #	client.message("You achieved 10 kills!!")
            #elif kills == 15:
            #	client.message("You achieved 15 kills!!")
            #------------------
            message = self._killingspree_messages_dict.get(kills, None)
        
        # loosing spree check
        elif deaths != 0:
            message = self._loosingspree_messages_dict.get(deaths, None)
            
        return message

    def show_message(self, client, victim=None, message=None):
        """\
        Replace variables and display the message
        """
        if (message != None) and not (client.hide):
            message = message.replace('%player%',client.name)
            if victim:
                message = message.replace('%victim%',victim.name)
            self.console.say(message)    
    
    def cmd_spree(self, data, client, cmd=None):
        """\	
        Show a players winning/loosing spree
        """        
        spreeStats = self.get_spree_stats(client)

        if spreeStats.kills > 0:
            cmd.sayLoudOrPM(client, '^7%s: ^7You have ^2%s^7 kill(s) in a row' % (client.name, spreeStats.kills))
            cmd.sayLoudOrPM(client, '^55 ^6Kills ^1-> ^5Superknife')
            cmd.sayLoudOrPM(client, '^510 ^6Kills ^1-> ^5HE Grenades')
            cmd.sayLoudOrPM(client, '^515 ^6Kills ^1-> ^520 Beretta Bullets')
        elif spreeStats.deaths > 0:
            cmd.sayLoudOrPM(client, '^7%s: ^7You have ^1%s^7 death(s) in a row' % (client.name, spreeStats.deaths))
            cmd.sayLoudOrPM(client, '^55 ^6Kills ^1-> ^5Superknife')
            cmd.sayLoudOrPM(client, '^510 ^6Kills ^1-> ^5HE Grenades')
            cmd.sayLoudOrPM(client, '^515 ^6Kills ^1-> ^520 Beretta Bullets')
        else:
            cmd.sayLoudOrPM(client, '^7%s: ^7You\'re not having a spree right now' % client.name)
            cmd.sayLoudOrPM(client, '^55 ^6Kills ^1-> ^5Superknife')
            cmd.sayLoudOrPM(client, '^510 ^6Kills ^1-> ^5HE Grenades')
            cmd.sayLoudOrPM(client, '^515 ^6Kills ^1-> ^520 Beretta Bullets')

if __name__ == '__main__':
    # create a fake console which emulates B3
    # PYTHONPATH=~/Downloads/+ARCHIV/B3/source/b3 python ~/Downloads/+ARCHIV/B3/source/b3/b3/extplugins/spreemod.py
    from b3.fake import fakeConsole, joe, superadmin, simon
 
    myplugin = SpreemodPlugin(fakeConsole)
    print("\n\n * * * * * * * * * * * *  Tests starting below * * * * * * * * * * * * \n\n")
 
    # we call onStartup() as the real B3 would do
    myplugin.onStartup()
 
    # make superadmin connect to the fake game server on slot 0
    superadmin.connects(cid=0)
 
    # make joe connect to the fake game server on slot 1
    joe.connects(cid=1)
    simon.connects(cid=2)
 
    # superadmin put joe in group user
    superadmin.says('!putgroup joe user')
    superadmin.says('!leveltest joe')
 
    # make joe try out again
    joe.says('!spree')
    
    # make joe achive some killingsprees
    joe.kills(simon)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
    joe.kills(superadmin)
 	
 	# make joe test spree again
    joe.says('!spree')