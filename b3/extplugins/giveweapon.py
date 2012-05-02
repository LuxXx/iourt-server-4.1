__version__ = '1.0'
__author__  = 'W33D|S0lll0s' # Thank you S0lll0s :D
 
import b3
import b3.events
import b3.plugin

class GiveweaponPlugin(b3.plugin.Plugin):
    requiresConfigFile = False
    
    def onStartup(self):
        # get the admin plugin so we can register commands
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            # something is wrong, can't start without admin plugin
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'giveweapon', 80, self.cmd_getweapon, 'gw')
        
    def cmd_getweapon(self, data, client=None, cmd=None):
        weaponids = {"none": "-@","knife": "A", "beretta": "B", "de": "C", "desert eagle": "C", "spas": "D", "mp5": "E", "ump": "F", "hk96": "G", "lr300": "f", "g36": "I", "psg": "J", "nade": "K", "he": "K", "grenade": "K", "smoke": "M", "smoke nade": "M", "smoke grenade": "M", "sr8": "N", "ak": "O", "kalashnikov": "O", "negev": "Q", "noob": "Q", "m4": "R", "m4a": "R", "m4a1": "R"}
        input = self._adminPlugin.parseUserCmd(data)
        if not data:
            client.message('^7 correct syntax is !giveweapon <player> <weapon id or name> [ammo] [clips]')
            return False
        else:
            if  len([x for x in data if x.isspace()]) < 1:
                client.message('^7 correct syntax is !giveweapon <player> <weapon id or name> [ammo] [clips]')
            else:
                input = data.split(' ',1)
                scname = input[0]
                weapon = input[1]
                if weapon in weaponids:
                   weapon = weaponids[input[1]]
                sclient = self._adminPlugin.findClientPrompt(scname, client)
                if not sclient: return False
                self.console.write("gw %s %s" % (sclient.cid, weapon))
        return True