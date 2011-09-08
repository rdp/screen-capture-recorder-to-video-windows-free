require 'win32ole'

module WMI

  # Generic WMI exception class.
  class WMIError < StandardError
  end

  # Invalid Class exception class.
  class InvalidClass < WMIError
  end

  # Invalid Query exception class.
  class InvalidQuery < WMIError
  end

  # Returns an array conating all the WMI subclasses
  # on a sytem.  Defaults to localhost
  #
  #   WMI.subclasses
  #   => ["Win32_PrivilegesStatus", "Win32_TSNetworkAdapterSettingError", ...]
  #
  #  For a more human readable version of subclasses when using options:
  #
  #   WMI.subclasses_of(:host => some_computer)
  #   => ["Win32_PrivilegesStatus", "Win32_TSNetworkAdapterSettingError", ...]
  def subclasses(options ={})
    Base.set_connection(options)
    b = Base.send(:connection)
    b.SubclassesOf.map { |subclass| class_name = subclass.Path_.Class }
  end


  alias :subclasses_of :subclasses

  extend self

  class Base
  # Many of the methods in Base are borrowed directly, or with some modification from ActiveRecord
  #   http://api.rubyonrails.org/classes/ActiveRecord/Base.html

    class << self
    
      # #find_by_wql currently only works when called through #find
      # it may stay like that too.  I haven't decided.
      def find_by_wql(query)
        d = connection.ExecQuery(query)
        begin
          d.count # needed to check for errors.  Weird, but it works.
        rescue => error
          case error.to_s
          when /Invalid class/i ; raise InvalidClass
          when /Invalid query/i ; raise InvalidQuery

          end
        end
        d.to_a
      end

      #  WMI::Win32ComputerSystem.find(:all)
      #    returns an array of Win32ComputerSystem objects
      #
      #  WMI::Win32ComputerSystem.find(:first)
      #    returns the first Win32ComputerSystem object
      #
      #  WMI::Win32ComputerSystem.find(:last)
      #    returns the last Win32ComputerSystem object
      #
      #  options:
      #
      #    :conditions
      #
      #     Conditions can either be specified as a string, array, or hash representing the WHERE-part of an SQL statement.
      #     The array form is to be used when the condition input is tainted and requires sanitization. The string form can
      #     be used for statements that don't involve tainted data. The hash form works much like the array form, except
      #     only equality and range is possible. Examples:
      #
      #       Win32ComputerSystem.find(:all, :conditions => {:drivetype => 3} )  # Hash
      #       Win32ComputerSystem.find(:all, :conditions => [:drivetype, 3] )   # Array
      #       Win32ComputerSystem.find(:all, :conditions => 'drivetype = 3' )   # String
      #
      #    :host       - computername, defaults to localhost
      #    :class      - swebm class , defaults to 'root\\cimv2'
      #    :privileges - see WMI::Privilege for a list of privileges
      #    :user       - username (domain\\username)
      #    :password   - password
      def find(arg=:all, options={})
        set_connection options
        case arg
          when :all; find_all(options)
          when :first; find_first(options)
          when :last; find_last(options)
        end
      end

      # an alias for find(:last)      
      def last(options={})
        find(:last, options)
      end
      
      # an alias for find(:first)      
      def first(options={})
        find(:first, options)
      end
      
      # an alias for find(:all)
      def all(options={})
        find(:all, options)
      end

      def set_connection(options={})
        @host = options[:host]
        @klass = options[:class] || 'root\\cimv2'
        @user,@password = options[:user], options[:password]
        @privileges = options[:privileges]
      end
      
      def set_wmi_class_name(name)
        @subclass_name = name
      end

      def subclass_name
        @subclass_name ||= self.name.split('::').last
      end

    private

      def connection
        @c ||= WIN32OLE.new("WbemScripting.SWbemLocator")
        @privileges.each { |priv| @c.security_.privileges.add(priv, true) } if @privileges
        @c.ConnectServer(@host,@klass,@user,@password)
      end

      def find_first(options={})
        find_all(options).first
      end
      
      def find_last(options={})
        find_all(options).last
      end

      def find_all(options={})
        find_by_wql(construct_finder_sql(options))
      end

      def construct_finder_sql(options)
        #~ scope = scope(:find)
        sql  = "SELECT #{options[:select] || '*'} "
        sql << "FROM #{options[:from] || subclass_name} "

        #~ add_joins!(sql, options, scope)
        add_conditions!(sql, options[:conditions], nil)

        sql << " GROUP BY #{options[:group]} " if options[:group]

        #~ add_order!(sql, options[:order], scope)
        #~ add_limit!(sql, options, scope)
        #~ add_lock!(sql, options, scope)

        sql
      end

      def add_conditions!(sql, conditions, scope = :auto)
        #~ scope = scope(:find) if :auto == scope
        segments = []
        segments << sanitize_sql(conditions)  unless conditions.nil?
        #~ segments << conditions unless conditions.nil?
        #~ segments << type_condition unless descends_from_active_record?
        segments.compact!
        sql << "WHERE #{segments.join(") AND (")} " unless segments.empty?
        sql.gsub!("\\","\\\\\\")
      end

      # Accepts an array, hash, or string of sql conditions and sanitizes
      # them into a valid SQL fragment.
      #   ["name='%s' and group_id='%s'", "foo'bar", 4]  returns  "name='foo''bar' and group_id='4'"
      #   { :name => "foo'bar", :group_id => 4 }  returns "name='foo''bar' and group_id='4'"
      #   "name='foo''bar' and group_id='4'" returns "name='foo''bar' and group_id='4'"
      def sanitize_sql(condition)
        case condition
          when Array; sanitize_sql_array(condition)
          when Hash;  sanitize_sql_hash(condition)
          else        condition
        end
      end

      # Sanitizes a hash of attribute/value pairs into SQL conditions.
      #   { :name => "foo'bar", :group_id => 4 }
      #     # => "name='foo''bar' and group_id= 4"
      #   { :status => nil, :group_id => [1,2,3] }
      #     # => "status IS NULL and group_id IN (1,2,3)"
      def sanitize_sql_hash(attrs)
        conditions = attrs.map do |attr, value|
          #~ "#{table_name}.#{connection.quote_column_name(attr)} #{attribute_condition(value)}"
          "#{attr} = '#{value}'"
        end.join(' AND ')

        #~ replace_bind_variables(conditions, attrs.values)
      end
    end
  end

  private

  def const_missing(name)
    self.const_set(name, Class.new(self::Base))
  end

  # allow for "WMI.Win32_Disk" style access
  def self.method_missing(m, *args)
   eval("WMI::" + m)
  end
end
