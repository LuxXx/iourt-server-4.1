__version__ = '1.0'
__author__  = 'W33D|S0lll0s' # Thank you S0lll0s :D

import b3, time, thread, threading, reimport b3.eventsimport b3.pluginimport b3.cronfrom b3.functions import soundex, levenshteinDistanceimport osimport randomimport stringimport traceback
class MapPlugin(b3.plugin.Plugin):
   requiresConfigFile = False
    
   def onStartup(self):
        # get the admin plugin so we can register commands
        self._adminPlugin = self.console.getPlugin('admin')
 
        if not self._adminPlugin:
            # something is wrong, can't start without admin plugin
            self.error('Could not find admin plugin')
            return
 
        # Register commands
        self._adminPlugin.registerCommand(self, 'changemap', 20, self.cmd_setmap, 'cmap')
        
   def cmd_setmap(self, data, client, cmd=None):
    """\
    <map> - switch current map
    """
    if not data:
        client.message('^7You must supply a map to change to.')
        return
    match = self.getMapsSoundingLike(data)
    if len(match) > 1:
        client.message('do you mean : %s' % string.join(match,', '))
        return True
    if len(match) == 1:
        mapname = match[0]
    else:
        client.message('^7cannot find any map like [^4%s^7].' % data)
        return False

    self.console.say('^7Changing map to %s' % mapname)
    time.sleep(1)
    self.console.write('map2 %s' % mapname)
    return True
    
    
   def getMapsSoundingLike(self, mapname):
    maplist = self.console.getMaps()
    data = mapname.strip()

    soundex1 = soundex(string.replace(string.replace(data, 'ut4_',''), 'ut_',''))
    #self.debug('soundex %s : %s' % (data, soundex1))

    match = []
    if data in maplist:
        match = [data]
    else:
        for m in maplist:
            s = soundex(string.replace(string.replace(m, 'ut4_',''), 'ut_',''))
            #self.debug('soundex %s : %s' % (m, s))
            if s == soundex1:
               #self.debug('probable map : %s', m)
               match.append(m)

    if len(match) == 0:
        # suggest closest spellings
        shortmaplist = []
        for m in maplist:
            if m.find(data) != -1:
                shortmaplist.append(m)
        if len(shortmaplist) > 0:
            shortmaplist.sort(key=lambda map: levenshteinDistance(data, string.replace(string.replace(map.strip(), 'ut4_',''), 'ut_','')))
            self.debug("shortmaplist sorted by distance : %s" % shortmaplist)
            match = shortmaplist[:3]
        else:
            maplist.sort(key=lambda map: levenshteinDistance(data, string.replace(string.replace(map.strip(), 'ut4_',''), 'ut_','')))
            self.debug("maplist sorted by distance : %s" % maplist)
            match = maplist[:3]
    return match
