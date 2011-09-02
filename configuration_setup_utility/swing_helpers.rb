=begin
Copyright 2010, Roger Pack 
This file is part of Sensible Cinema.

    Sensible Cinema is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Sensible Cinema is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Sensible Cinema.  If not, see <http://www.gnu.org/licenses/>.
=end
require 'java'

module SwingHelpers 
  
 include_package 'javax.swing'
 [JProgressBar, JButton, JFrame, JLabel, JPanel, JOptionPane,
   JFileChooser, JComboBox, JDialog, SwingUtilities, JSlider] # grab these constants (http://jira.codehaus.org/browse/JRUBY-5107)
 include_package 'java.awt'
 [FlowLayout, Font, BorderFactory, BorderLayout]
 include_class java.awt.event.ActionListener
 JFile = java.io.File
 include_class java.awt.FileDialog
 include_class java.lang.System
 UIManager

 class JOptionPane
    JOptionReturnValuesTranslator = {0 => :yes, 1 => :no, 2 => :cancel, -1 => :exited}
    
    # accepts :yes => "yes text", :no => "no text"
    # returns  :yes :no :cancel: or :exited
    def self.show_select_buttons_prompt message, names_hash = {}
      old = ['no', 'yes', 'ok'].map{|name| 'OptionPane.' + name + 'ButtonText'}.map{|name| [name, UIManager.get(name)]}
      if names_hash[:yes]
        UIManager.put("OptionPane.yesButtonText", names_hash[:yes])
      end
      if names_hash[:no]
        UIManager.put("OptionPane.noButtonText", names_hash[:no])
      end
      # if names_hash[:ok] # ???
      #   UIManager.put("OptionPane.okButtonText", names_hash[:ok])
      # end
      if names_hash[:cancel]
        UIManager.put("OptionPane.noButtonText", names_hash[:cancel])
      end
      title = message.split(' ')[0..5].join(' ')
      returned = JOptionPane.showConfirmDialog nil, message, title, JOptionPane::YES_NO_CANCEL_OPTION # LODO self?
      old.each{|name, old_setting| UIManager.put(name, old_setting)}
      JOptionReturnValuesTranslator[returned]
    end
    
end

 class JButton
   def initialize *args
    super *args
    set_font Font.new("Tahoma", Font::PLAIN, 11)
   end
  
   def on_clicked &block
     raise unless block
     @block = block
     add_action_listener do |e|
       begin
         block.call
       rescue Exception => e
         puts 'got fatal exception thrown in button [ignoring]', e, e.class
        if $VERBOSE
         puts e.backtrace, e
        else
         puts e.backtrace[0], e
        end
       end        
     end
     self
   end
  
   def simulate_click
     @block.call
   end
  
   def tool_tip= text
      if text
       text = "<html>" + text + "</html>"
       text = text.gsub("\n", "<br/>")
     end
     self.set_tool_tip_text text   
   end
  
 end

 ToolTipManager.sharedInstance().setDismissDelay(10000)

 class JFrame
   def close
     dispose # sigh
   end
  end
  
  # wrapped in sensible-cinema-base
  class JFileChooser
    # also set_current_directory et al...
    
    # raises on failure...
    def go
      success = show_open_dialog nil
      unless success == Java::javax::swing::JFileChooser::APPROVE_OPTION
        java.lang.System.exit 1 # kills background proc...but we shouldn't let them do stuff while a background proc is running, anyway
      end
      get_selected_file.get_absolute_path
    end
    
    # match FileDialog methods...
    def set_title x
      set_dialog_title x
    end
    
    def set_file f
      set_selected_file JFile.new(f)
    end
    
    alias setFile set_file

    # choose a file that may or may not exist yet...
    def self.new_nonexisting_filechooser_and_go title = nil, default_dir = nil, default_file = nil
      out = JFileChooser.new
      out.set_title title
      if default_dir
        out.set_current_directory JFile.new(default_dir)
      end
      if default_file
        out.set_file default_file
      end
      out.go
    end
    
  end
  
  # awt...the native looking one...
  class FileDialog
    def go
      show
      File.expand_path(get_directory + '/' + get_file) if get_file # get_file implies they picked something...
    end
    
    # this actually allows for non existing files [oopsy] LODO
    def self.new_previously_existing_file_selector_and_go title, use_this_dir = nil
      out = FileDialog.new(nil, title, FileDialog::LOAD) # LODO no self in here... ?
      out.set_title title
      if use_this_dir
        # FileDialog only accepts paths a certain way...
        dir = File.expand_path(use_this_dir).gsub(File::Separator, File::ALT_SEPARATOR)
        out.setDirectory(dir) 
      end
      Thread.new { sleep 2; out.to_front } # it gets hidden, unfortunately, so try and bring it again to the front...
      #out.remove_notify # allow our app to exit [?]
      got = out.go
      raise 'must exist' unless File.exist? go
      raise 'cancelled choosing existing file' unless got # I think we always want to raise...
      got
    end
    
  end
  
  class NonBlockingDialog < JDialog
    def initialize title_and_display_text, close_button_text = 'Close'
      super nil
      lines = title_and_display_text.split("\n")
      set_title lines[0]
      get_content_pane.set_layout nil
      lines.each_with_index{|line, idx|
        jlabel = JLabel.new line
        jlabel.set_bounds(10, 15*idx, 550, 24)
        get_content_pane.add jlabel
      }
      close = JButton.new( close_button_text ).on_clicked {
        self.dispose
      }
      close.set_bounds(125,30+15*lines.length,70,25)
      get_content_pane.add close
      set_size 550, 100+15*lines.length # XXX variable width? or use swing build in better?
      set_visible true
      setDefaultCloseOperation JFrame::DISPOSE_ON_CLOSE
      setLocationRelativeTo nil # center it on the screen
    end
  end
  
  def self.get_user_input(message, default = '', cancel_ok = false)
    received = javax.swing.JOptionPane.showInputDialog(message, default)
    unless cancel_ok
      raise 'user cancelled' unless received
    end
    received
  end
  
  def self.show_in_explorer filename_or_path
    p 'here1'
    raise 'nonexist' unless File.exist?(filename_or_path)
    p 'here2'
 #   begin
      p 'here3'
        c = "explorer /e,/select,\"#{File.expand_path(filename_or_path).to_filename}\"" 
        p c
        p 'here4'
        system c # command returns immediately...so system is ok
#    rescue => why_does_this_happen_ignore_this_exception_it_probably_actually_succeeded
#    end
  end

end

class String
 def to_filename
  if File::ALT_SEPARATOR
    self.gsub('/', File::ALT_SEPARATOR)
  else
    self
  end
 end
end
