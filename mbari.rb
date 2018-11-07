####################  mbari.rb -- brent@mbari.org  #####################
#
#  MBARI Generic (application-independent) utilites -- revised: 11/7/18
#
########################################################################

require 'ruby2compat'

begin
  require 'mbarilib'  #Kernel.doze method
rescue LoadError
  if Kernel.respond_to? :sleep!
    STDERR.puts "Warning:  missing mbarilib extension -- doze == sleep!"
    module Kernel
      alias_method :doze, :sleep!
    end
  else
    STDERR.puts "Warning:  missing mbarilib extension -- broken doze method"
    def doze duration
      Thread.critical=false
      sleep duration
    end
  end
end


class String
  alias_method :/, :[]
end


class Module
  private
  def rename_method newId,oldId
    alias_method newId, oldId unless method_defined? newId
  end
end


class Object
  alias_method :klass, :class

  def intern  #Symbol class overrides this. All classes respond to it
    self
  end
  def intern= identifier  #return identifier for Object.intern
    define_singleton_method(:intern) {identifier}
    identifier
  end

  def deepCopy
    Marshal::load(Marshal::dump(dup))
  end

  def reallyEqual? other  #for recursive equality tests (deprecated)
    self == other
  end

  def with hash
  #assign instance variables specified in given hash
    hash.each do |parameter, value|
      send("#{parameter}=".to_sym, value)
    end
    self
  end
end
