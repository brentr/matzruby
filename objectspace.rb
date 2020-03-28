#################  objectspace.rb -- brent@mbari.org  ##################
#  ObjectSpace enhancements (useful for debugging and introspection) 
########################################################################

require 'mbari'

module ObjectSpace
  def self.each (classOrMod=Object, &block)
  #first remove unref'd garbage, then iterate over the objects
    garbage_collect
    each_object (classOrMod, &block)
  end
  
  def self.each_reference (target, klass=nil, &block)
  #execute block with each object that references target
  #useful if you can't figure out why something won't GC
  #VERY SLOW!
    target = target.intern
    n=0
    block=proc {|mod, names| 
      puts "#{mod}#{names ? "::"+names.inspect : ""}"} unless block
    unless klass
      refs=global_variables
      refs.delete_if {|v| not eval(v.to_s).equal? target}
      n+=refs.size
      refs.each &block
      klass=Object
    end
    each(klass) {|obj|
      refs=obj.instance_variables
      refs.delete_if {|v| not obj.instance_variable_get(v).equal? target}
      if obj.kind_of? Module
        refs=obj.class_variables
        refs.delete_if{|v| not obj.__send__(:class_variable_get,v).equal? target}
        n+=refs.size
        refs.each &block
        refs=obj.constants_at
        refs.delete_if {|v| not obj.const_get(v).equal? target}
        n+=refs.size
        refs.each &block
      end
    }
    n
  end
end
 
