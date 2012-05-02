
__author__  = 'LuxXx'
__version__ = '0.1'


import b3
import b3.events
import b3.plugin
import string
import time, thread, threading
import re # for debug purpose only

#--------------------------------------------------------------------------------------------------
class RewardkillsPlugin(b3.plugin.Plugin):
    _adminPlugin = None
    _xlrstatsPlugin = None
    _minLevel = 100
    _nbHWK = 0 
    _nbTop = 5
    _msgLevels = set()
    _waterKillers = {}
    requiresConfigFile = False
 
    def onLoadConfig(self):
        self._adminPlugin = self.console.getPlugin('admin')
        if not self._adminPlugin:
            self.error('Could not find admin plugin')
            return False
        # else:
            # self._adminPlugin.debug('Plugin loaded')
        self._xlrstatsPlugin = self.console.getPlugin('xlrstats')
        if not self._xlrstatsPlugin:
            self.debug('Could not find xlrstats plugin')

        # Options loading
        try:
            self._minLevel = self.config.getint('settings', 'minlevel')
        except:
            pass
        self.debug('Minimum Level to use commands = %d' % self._minLevel)

        try:
            self._waterId = self.config.getint('settings', 'waterid')
        except:
            pass
        self.debug('water ID = %d' % self._waterId)
  
        self.query = self.console.storage.query
        
    def onStartup(self):
        self.registerEvent(b3.events.EVT_GAME_ROUND_START)
        self.registerEvent(b3.events.EVT_GAME_EXIT)
        # '-> See poweradmin 604
        self.registerEvent(b3.events.EVT_CLIENT_SUICIDE)
        self.registerEvent(b3.events.EVT_CLIENT_KILL)
    
    def onEvent(self, event):
        if event.type == b3.events.EVT_CLIENT_SUICIDE:
            self.someoneKilled(event.client, event.target, event.data)
        if event.type == b3.events.EVT_CLIENT_KILL: 
            self.someoneKilled(event.client, event.target, event.data)
      # elif (event.type == b3.events.EVT_GAME_EXIT) or (event.type == b3.events.EVT_GAME_ROUND_START):
       
    def resetScores(self):
        self._nbHWK = 0
        self._waterKillers = {}
        
       	
            
    def someoneKilled(self, client, target, data=None):
        if data[1] == self.console.UT_MOD_KNIFE:
            self._nbHWK += 1
            if self._nbHWK + 0:
             	self.console.write('gh %s +20' % (client.cid))
                self.console.write('scc %s print "^5You are rewarded ^620 ^5percent of health because of your knife kill"' % (client.cid))
        
        if data[1] == self.console.UT_MOD_GOOMBA:
            self._nbHWK += 1
            if self._nbHWK + 0:
                self.console.write('gh %s +100' % (client.cid))
                self.console.write('scc %s print "^5You are rewarded ^6100 ^5percent of health because of your goomba kill"' % (client.cid))