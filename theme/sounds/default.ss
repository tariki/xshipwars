#    ShipWars: Sound Scheme File
#
#	This file contains a list of sound referances that
#	correspond to a sound file.
#
#
#	Format of this file is as follows:
#
#	Title = s		Specifies the title for this entire
#				Sound Scheme.
#
#	AudioModeName = s	Specifies the Y2 compliant Audio mode
#				name for use on all the sound files for
#				this entire Sound Sceheme.
#
#				If you are not sure what value to give,
#				the default Audio mode name `Default' is
#				always defined on the sound server. Other
#				possible Audio mode names are:
#
#				PlayMono8000
#				PlayStereo8000
#				PlayMono11025
#				PlayStereo11025
#				PlayMono22050
#				PlayStereo22050
#				PlayMono44100
#				PlayStereo44100
#
#
#	BeginSoundItem = i	Marks the beginning of a Sound Item
#				configuration block for sound item
#				number i.
#                               
#	EndSoundItem		Marks the end of a Sound Item
#                               configuration block.
#
#
#	SoundItem configuration block members:
#
#	FileName = s	Specifies the path to the sound file associated
#			with this sound item. If the path is not
#			absolute then the SoundsDir (specified
#			in the client configuration) will be prepended
#			to it.
#
#
#

# #####################################################################
#
#   Header
#

#   Title of this sound scheme:
#
Title = XShipWars Default


#   Sound server mode name:
#
#	Name of the mode the sound server is to switch to when
#	using this sound scheme.
#
ModeName = PlayStereo11025



# ##################################################################### 
#
#   Sound scheme items:
#
#	Some sound items require specific index numbers, do not change
#	the index numbers for those sound items.
#

# All purpose default sound when no other sound is available.
BeginSoundItem = 0
    Filename = button01.wav
EndSoundItem

# Standard button press beep.
BeginSoundItem = 1
    Filename = button01.wav
EndSoundItem

# Standard error beep.
BeginSoundItem = 2
    Filename = stderr01.wav
EndSoundItem

# "The XSW Sound".
BeginSoundItem = 3
    Filename = xsw_logo01.wav
EndSoundItem

# Menu item highlight.
BeginSoundItem = 4
    Filename = menu_highlight01.wav
EndSoundItem

# Menu item select.
BeginSoundItem = 5
    Filename = menu_select01.wav
EndSoundItem


#   Low attention beep.
#
BeginSoundItem = 10
    Filename = attention01.wav
EndSoundItem

#   Moderate attention beep.
#
BeginSoundItem = 11
    Filename = attention01.wav
EndSoundItem

#   Loud attention beep.
#
BeginSoundItem = 12
    Filename = attention01.wav
EndSoundItem


#   Incomming message beep.
#
BeginSoundItem = 15
    Filename = attention01.wav
EndSoundItem


#   Object scan beep.
#
BeginSoundItem = 16
    Filename = scan01.wav
EndSoundItem

#   Contacts beep.
#
BeginSoundItem = 17
    Filename = contacts01.wav
EndSoundItem

#   Hail outgoing.
#
BeginSoundItem = 18
    Filename = hail_outgoing.wav
EndSoundItem

#   Hail incoming.
#   
BeginSoundItem = 19
    Filename = hail_incoming.wav
EndSoundItem


# ######################################################################
#   
#   Operations sounds:
#
#       Do not change the index values for any of the sound
#       items in this section, they are standard.
#

#   Engines on.
#
BeginSoundItem = 20
    Filename = power_up.wav
EndSoundItem

#   Engines off.
#
BeginSoundItem = 21
    Filename = power_down.wav
EndSoundItem

#   Cloak up.
#
BeginSoundItem = 22
    Filename = cloak_up.wav
EndSoundItem

#   Cloak down.
#
BeginSoundItem = 23
    Filename = cloak_down.wav
EndSoundItem



# ######################################################################
#
#   Weapons fire sounds:
#
#	Do not change the index values for any of the sound
#	items in this section, they are standard.
#

# Stream weapons fire.
BeginSoundItem = 30
    Filename = fire_stream01.wav
EndSoundItem

# Pulse weapons fire.
BeginSoundItem = 31
    Filename = fire_pulse01.wav
EndSoundItem

# Projectile weapons fire.
BeginSoundItem = 32
    Filename = fire_projectile01.wav
EndSoundItem


# ######################################################################
#
#   Explosion sounds:
#
#       Do not change the index values for any of the sound
#       items in this section, they are standard.
#

# Minor collision sound.
BeginSoundItem = 50
    Filename = collide01.wav
EndSoundItem

# Weapons fire hit target shields.
BeginSoundItem = 53
    Filename = hit_target01.wav
EndSoundItem

# Weapons fire hit target structure.
BeginSoundItem = 54
    Filename = hit_target01.wav
EndSoundItem

# Weapons fire hit our shields.
BeginSoundItem = 55
    Filename = hit_us01.wav
EndSoundItem

# Weapons fire hit our structure.
BeginSoundItem = 56
    Filename = hit_us01.wav
EndSoundItem

# Explosion or detonation of a vessel.
BeginSoundItem = 57
    Filename = hit_target02.wav
EndSoundItem

# Planet or starbase exploding.
BeginSoundItem = 58
    Filename = hit_target02.wav
EndSoundItem


# ######################################################################
#
#   Background music sounds:
#
#       Do not change the index values for any of the sound
#       items in this section, they are standard.
#

# Standard `no mood' background music.
BeginSoundItem = 100
    Filename = bluedanube.mid
EndSoundItem

# Exploring mood.
BeginSoundItem = 101
    Filename = bluedanube.mid
EndSoundItem

# Fighting mood.
BeginSoundItem = 102
    Filename = marsbringerofwar.mid
EndSoundItem

# Mysty (nebula) mood.
BeginSoundItem = 103
    Filename = aquarium.mid
EndSoundItem
