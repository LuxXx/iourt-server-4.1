__version__ = '1.0'
__author__  = 'W33D|S0lll0s' # Thank you S0lll0s :D
 
import b3
import b3.events
import b3.plugin

class TeleportPlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        # get the admin plugin so we can register commands
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            # something is wrong, can't start without admin plugin
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'teleport', 40, self.cmd_teleport, 'tp')
        
    def cmd_teleport(self, data, client=None, cmd=None):
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !teleport <playername or partialname> <playername or partialname>')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 1:
                client.message('^7 correct syntax is !teleport <playername or partialname> <playername or partialname>')
            else:
                input = data.split(' ',1)
                scname = input[0]
                ccname = input[1]
             	sclient = self._adminPlugin.findClientPrompt(scname, client)
                cclient = self._adminPlugin.findClientPrompt(ccname, client)
                if not sclient: return False
                self.console.write("teleport %s %s" % (sclient.cid, cclient.cid))
                self.console.write('bigtext "^1%s ^2was teleported to ^1%s"' % (sclient.exactName, cclient.exactName))
        return True