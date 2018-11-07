##################  ruby2compat.rb -- brent@mbari.org  ###################
#
#  Try to make Ruby 1.8 more compatible with Ruby 1.9+
#
##########################################################################


class Module
  def constants_at
  #return Array of names of constants defined in specified module
    acs = ancestors
    cs = constants
    if acs.length > 1
      acs[1..-1].each do |ac|
        both = ac.constants & cs
        both = both.select{|c| ac.const_get(c) == const_get(c)}
        cs -= both if both
      end
    end
    cs
  end
end


class Encoding < String
#all Strings are 8-bit ASCII
  US_ASCII = new('US-ASCII').freeze
  ASCII_8BIT = new('ASCII-8BIT').freeze
  BINARY = new('BINARY').freeze
  @@names = [BINARY, ASCII_8BIT, US_ASCII].freeze
  def name
    self
  end
  def self.list
    @@names
  end
  def self.name_list
    @@names
  end
  def names
    @@names
  end
  def ascii_compatible?
    true
  end
end


class Fixnum
  def ord
    self
  end
end


class String
  def ord
    self[0]
  end

  def force_encoding desired
    warn "#{caller}\nRuby #{RUBY_VERSION} does not support String Encoding #{
      desired.inspect}" unless Encoding.list.include? desired.upcase
    self
  end
end


class Object
  def singleton_class
    class <<self
      self
    end
  end

  def define_singleton_method *args, &block
    singleton_class.__send__ :define_method, *args, &block
  end

  def singleton_method symbol
    return method symbol if singleton_methods(false).include? symbol
    raise NameError,
      "undefined singleton method \`#{symbol}\' for #{inspect}:#{self.class}"
  end
end


class Class  #create an uninitialized class instance
#see http://whytheluckystiff.net/articles/rubyOneEightOh.html
  def allocate
    class_name = to_s
    Marshal.load "\004\006o:"+(class_name.length+5).chr+class_name+"\000"
  end unless method_defined? :allocate
end


class <<IO
  def write name, string, offset=nil
    File.open name, offset ? "r+":"w" do |output|
      output.seek offset if offset
      output.write string
    end
  end
  alias_method :binwrite, :write
  alias_method :binread, :read
end
