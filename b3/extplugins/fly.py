__version__ = '1.0'
__author__  = 'LuxXx'
 
import b3
import b3.events
import b3.plugin

class FlyPlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        # get the admin plugin so we can register commands
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            # something is wrong, can't start without admin plugin
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'fly', 80, self.cmd_fly, 'unfly')
        
    def cmd_fly(self, data, client=None, cmd=None):
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !fly [playername or partialname]')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 0:
                client.message('^7 correct syntax is !fly [playername or partialname]')
            else:
                input = data.split(' ',1)
                scname = input[0]
             	sclient = self._adminPlugin.findClientPrompt(scname, client)
                if not sclient: return False
                self.console.write("sv_cheats 1")
                self.console.write("spoof %s noclip" % (sclient.cid))
                self.console.write("sv_cheats 0")
        return True