__version__ = '1.0'
__author__  = 'LuxXx'
 
import b3
import b3.events
import b3.plugin

class InvisiblePlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'invisible', 80, self.cmd_invisible, 'inv')
        
    def cmd_invisible(self, data, client=None, cmd=None):
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !invisible <playername or partialname>')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 0:
                client.message('^7 correct syntax is !invisible <playername or partialname>')
            else:
                input = data.split(' ',1)
                cname = input[0]
                sclient = self._adminPlugin.findClientPrompt(cname, client)
                if not sclient: return False
                self.console.write('invisible %d' % (sclient.cid)
                self.console.write('sendclientcommand all print "%s ^7disconnected"' % (sclient.exactName))
                self.console.write('sendclientcommand %s cp "^2No one can see you."' % (sclient.cid))
                self.console.write('fattext %s ^1You are invisible!' % (sclient.cid))
        return True