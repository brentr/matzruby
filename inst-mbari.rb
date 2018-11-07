#!/usr/bin/env ruby

BIN_IRB_TOOLS = []
LIB_IRB_TOOLS = ["ruby2compat.rb", "mbari.rb", "objectspace.rb", "sourceref.rb"]

require "rbconfig.rb"
include Config

destdir = ARGV[0] || ''

sitebindir = destdir+CONFIG["sitearchdir"]
sitelibdir = destdir+CONFIG["sitelibdir"]

puts sitebindir,sitelibdir

$:.unshift CONFIG["srcdir"]+File::Separator+"lib" if CONFIG["srcdir"]

require "ftools"

for f in BIN_IRB_TOOLS
  File.install f, sitebindir+File::Separator+File.basename(f, '.rb'), 0755, true
end


for f in LIB_IRB_TOOLS
  File.install f, sitelibdir+File::Separator+File.basename(f), 0644, true
end


