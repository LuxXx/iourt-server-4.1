__version__ = '1.0'
__author__  = 'LuxXx'
 
import b3
import b3.events
import b3.plugin

class FattextPlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'fattext', 40, self.cmd_fattext, 'ft')
        
    def cmd_fattext(self, data, client=None, cmd=None):
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !fattext <text>')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 0:
                client.message('^7 correct syntax is !fattext <text>')
            else:
                input = data.split(' ',1)
                self.console.write('sendclientcommand all cs 28 "%s"' % (data))
        return True