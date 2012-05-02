__version__ = '1.0'
__author__  = 'W33D|S0lll0s' # Thank you S0lll0s :D
 
import b3
import b3.events
import b3.plugin

class RenamePlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'setname', 40, self.cmd_setname, 'name')
        
    def cmd_setname(self, data, client=None, cmd=None):
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !setname <playername or partialname> <new name>')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 1:
                client.message('^7 correct syntax is !setname <playername or partialname> <new name>')
            else:
                input = data.split(' ',1)
                cname = input[0]
                newname = input[1]
                sclient = self._adminPlugin.findClientPrompt(cname, client)
                if not sclient: return False
                self.console.write("forcecvar %s name %s" % (sclient.cid, newname))
        return True