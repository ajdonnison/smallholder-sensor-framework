"""
 Config handling routines for SGF project
"""

import sys
import re
import os.path
import ConfigParser

_CFG = {}

def get(**options):
  if _CFG:
    return _CFG

  cfg = ConfigParser.RawConfigParser(allow_no_value=True)
  if not options['cfgfile']:
    print "Must supply config file"
    sys.exit(1)
  
  try:
    cfg.read(options['cfgfile'])
    for sect in cfg.sections():
      _CFG[sect] = dict(cfg.items(sect))
      for key in _CFG[sect]:
        if _CFG[sect][key] and _CFG[sect][key].isdigit():
          _CFG[sect][key] = int(_CFG[sect][key])

  except ConfigParser.Error:
    print "Invalid configuration file - aborting"
    sys.exit(1)
  return _CFG

def test_cfg():
  cfg = get(cfgfile="test.cfg")
  for sect in cfg:
    for key in cfg[sect]:
      print sect + ":" + key + "=" + str(cfg[sect][key])

if __name__ == "__main__":
  test_cfg()

# vi:sw=2 ai expandtab:
