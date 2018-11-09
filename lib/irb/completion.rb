#
#   irb/completion.rb -
#   	Release Version: 0.9
#   	by Keiju ISHITSUKA(keiju@ishitsuka.com)
#       From Original Idea of shugo@ruby-lang.org
#       Revised:  11/7/18  brent@mbari.org
#

module IRB
  module InputCompletor

    ReservedWords = [
      :BEGIN, :END,
      :alias, :and,
      :begin, :break,
      :case, :class,
      :def, :defined, :do,
      :else, :elsif, :end, :ensure,
      :false, :for,
      :if, :in,
      :module,
      :next, :nil, :not,
      :or,
      :redo, :rescue, :retry, :return,
      :self, :super,
      :then, :true,
      :undef, :unless, :until,
      :when, :while,
      :yield
    ]

    CompletionProc = proc { |*arg|
      input = arg[0]
      bind = arg[1] || IRB.conf[:MAIN_CONTEXT].workspace.binding

#puts "\ninput: #{input}\n"

      case input
      when /^(\/[^\/]*\/)\.([^.]*)$/
	# Regexp
	receiver = $1
	message = Regexp.quote($2)
	select_message(receiver, message, Regexp.instance_methods)

      when /^([^\]]*\])\.([^.]*)$/
	# Array
	receiver = $1
	message = Regexp.quote($2)
	select_message(receiver, message, Array.instance_methods)

      when /^([^\}]*\})\.([^.]*)$/
	# Proc or Hash
	receiver = $1
	message = Regexp.quote($2)
	select_message(receiver, message,
          Proc.instance_methods | Hash.instance_methods)

      when /^(:[^:.]*)$/
 	# Symbol
	if Symbol.respond_to?(:all_symbols)
	  sym = $1
	  Symbol.all_symbols.map!{|s| ":"<<s.id2name}.grep /^#{sym}/
	else
	  []
	end

      when /^::([A-Z][^:\.\(]*)$/
	# Absolute Constant or class methods
	receiver = $1
	Object.constants.map!(&:to_s).grep(/^#{receiver}/) {|e| "::"<<e}

      when /^(((::)?[A-Z][^:.\(]*)+)::?([^:.]*)$/
	# Constant or class methods
	receiver = $1
	message = Regexp.quote($4)
	begin
	  eval("#{receiver}.constants | #{receiver}.methods",bind).map!(&:to_s)
	rescue Exception
	  []
	end.grep(/^#{message}/) {|e| receiver + ("::"<<e)}

      when /^(:[^:.]+)\.([^.]*)$/
	# Symbol
	receiver = $1
	message = Regexp.quote($2)
	select_message(receiver, message, Symbol.instance_methods)

      when /^(-?(0[dbo])?[0-9_]+(\.[0-9_]+)?([eE]-?[0-9]+)?)\.([^.]*)$/
	# Numeric
	receiver = $1
	message = Regexp.quote($5)

	candidates = begin
	  eval(receiver, bind).methods
	rescue Exception
	  []
	end
	select_message(receiver, message, candidates)

      when /^(-?0x[0-9a-fA-F_]+)\.([^.]*)$/
	# Numeric(0xFFFF)
	receiver = $1
	message = Regexp.quote($2)

	candidates = begin
	  eval(receiver, bind).methods
	rescue Exception
	  []
	end
	select_message(receiver, message, candidates)

      when /^(\$[^.]*)$/
	global_variables.map!(&:to_s).grep Regexp.new(Regexp.quote($1))

#      when /^(\$?(\.?[^.]+)+)\.([^.]*)$/
      when /^((\.?[^.]+)+)\.([^.]*)$/
	# variable
	receiver = $1
	message = Regexp.quote($3)

	gv = global_variables
	lv = eval("local_variables", bind)
	cv = eval("self.class.constants", bind)

	if (gv | lv | cv).include? receiver.to_sym
	  # foo.func and foo is local var.
	  candidates = eval "#{receiver}.methods", bind
	elsif /^[A-Z]/ =~ receiver and /\./ !~ receiver
	  # Foo::Bar.func
	  begin
	    candidates = eval "#{receiver}.methods", bind
	  rescue Exception
	    candidates = []
	  end
	else
	  # func1.func2
	  candidates = []
	  ObjectSpace.each_object Module do |m|
	    name = m.name
	    candidates.concat m.instance_methods(false) unless
              name != "IRB::Context" and
              /^(IRB|SLex|RubyLex|RubyToken)/ =~ name
	  end
          candidates.uniq!
	end
	select_message receiver, message, candidates

      when /^\.([^.]*)$/
	# unknown(maybe String)

	receiver = ""
	message = Regexp.quote($1)
	select_message(receiver, message, String.instance_methods)

      else
	candidates = eval(<<-END, bind)
          methods | private_methods | local_variables | self.class.constants
        END
        candidates |= eval("Object.constants", bind) if self.class != Object

	(candidates|ReservedWords).map!(&:to_s).grep /^#{Regexp.quote(input)}/
      end
    }

    def self.select_message(receiver, message, candidates)
      matches=[]
      candidates.each do |sym|
        (e=sym.to_s).start_with? message and /^[a-zA-Z_]/ =~ e and
	  matches.push receiver + ("."<<e)
      end
      matches
    end

    def self.install
      if Readline.respond_to? :basic_word_break_characters=
        Readline.basic_word_break_characters= c = " \t\n\"\\'`><=;|&{(+-*/"
        if Readline.respond_to? :completer_word_break_characters=
          Readline.completer_word_break_characters= c<<?@
        end
      end
      Readline.completion_append_character = nil
      Readline.basic_quote_characters = ''
      Readline.completion_proc = IRB::InputCompletor::CompletionProc
    end
    install if defined? Readline
  end
end
