#			M O U S E . T C L
#
#	TCL macros for selecting among the solids/objects being displayed.
#
#	Author -
#		Robert Parker
#

proc mouse_get_spath { x y } {
    global mged_gui

    set win [winset]
    set id [get_player_id_dm $win]

    if {[info exists mged_gui($id,edit_menu)] && \
	    [winfo exists $mged_gui($id,edit_menu)]} {
	destroy $mged_gui($id,edit_menu)
    }

    set mged_gui($id,mgs_path) ""
    set ray [mouse_shoot_ray $x $y]
    set paths [ray_get_info $ray in path]

    if {![llength $paths]} {
	cad_dialog .$id.spathDialog $mged_gui($id,screen)\
		"Nothing was hit!"\
		"Nothing was hit!"\
		"" 0 OK
	return ""
    }

    if {[llength $paths] == 1} {
	return [lindex $paths 0]
    }

    set top .mgs$id
    if [winfo exists $top] {
	destroy $top
    }

    if [info exists mged_gui($id,screen)] {
	set screen $mged_gui($id,screen)
    } else {
	set screen [winfo screen $win]
    }

    create_listbox $top $screen Solid $paths "destroy $top"
    set mged_gui($id,edit_menu) $top

    bind_listbox $top "<ButtonPress-1>"\
	    "set item \[get_listbox_entry %W %x %y\];\
	    solid_illum \$item; break"
    bind_listbox $top "<Double-1>"\
	    "set mged_gui($id,mgs_path) \[get_listbox_entry %W %x %y\];\
	    destroy $top; break"
    bind_listbox $top "<ButtonRelease-1>"\
	    "%W selection clear 0 end; _mged_press reject; break"

    while {$mged_gui($id,mgs_path) == ""} {
	mged_update 0
    }

    return $mged_gui($id,mgs_path)
}

proc mouse_get_spath_and_pos { x y } {
    global mged_gui

    set win [winset]
    set id [get_player_id_dm $win]

    set mged_gui($id,mgs_path) [mouse_get_spath $x $y]

    if {$mged_gui($id,mgs_path) == ""} {
	return ""
    }

    set mged_gui($id,mgs_pos) -1
    set top .mgsp$id

    if [info exists mged_gui($id,screen)] {
	set screen $mged_gui($id,screen)
    } else {
	set screen [winfo screen $win]
    }
    regexp "\[^/\].*" $mged_gui($id,mgs_path) match
    set path_components [split $match /]
    create_listbox $top $screen Matrix $path_components "destroy $top"
    set mged_gui($id,edit_menu) $top

    bind_listbox $top "<ButtonPress-1>"\
	    "set item \[%W index @%x,%y\];\
	    _mged_press oill;\
	    _mged_ill \$mged_gui($id,mgs_path);\
	    _mged_matpick -n \$item; break"
    bind_listbox $top "<Double-1>"\
	    "set mged_gui($id,mgs_pos) \[%W index @%x,%y\];\
	    destroy $top; break"
    bind_listbox $top "<ButtonRelease-1>"\
	    "%W selection clear 0 end; _mged_press reject; break"

    while {$mged_gui($id,mgs_pos) == -1} {
	mged_update 0
    }

    return "$mged_gui($id,mgs_path) $mged_gui($id,mgs_pos)"
}

proc mouse_get_comb { x y } {
    global mged_gui

    set win [winset]
    set id [get_player_id_dm $win]

    if {[info exists mged_gui($id,edit_menu)] && \
	    [winfo exists $mged_gui($id,edit_menu)]} {
	destroy $mged_gui($id,edit_menu)
    }

    set mged_gui($id,mgc_comb) ""
    set ray [mouse_shoot_ray $x $y]
    set paths [ray_get_info $ray in path]
    if {![llength $paths]} {
	cad_dialog .$id.combDialog $mged_gui($id,screen)\
		"Nothing was hit!"\
		"Nothing was hit!"\
		"" 0 OK
	return ""
    }
    set combs [build_comb_list $paths]

    if {[llength $combs] == 1} {
	set mged_gui($id,mgc_comb) [lindex $combs 0]
	return $mged_gui($id,mgc_comb)
    }

    set top .mgc$id
    if [winfo exists $top] {
	destroy $top
    }

    if [info exists mged_gui($id,screen)] {
	set screen $mged_gui($id,screen)
    } else {
	set win [winset]
	set screen [winfo screen $win]
    }

    create_listbox $top $screen Combination $combs "destroy $top"
    set mged_gui($id,edit_menu) $top

    bind_listbox $top "<ButtonPress-1>"\
	    "set comb \[%W get @%x,%y\];\
	    set spath \[comb_get_solid_path \$comb\];\
	    set path_pos \[comb_get_path_pos \$spath \$comb\];\
	    matrix_illum \$spath \$path_pos; break"
    bind_listbox $top "<Double-1>"\
	    "set mged_gui($id,mgc_comb) \[%W get @%x,%y\];\
	    destroy $top; break"
    bind_listbox $top "<ButtonRelease-1>"\
	    "%W selection clear 0 end;\
	    _mged_press reject; break"

    while {$mged_gui($id,mgc_comb) == ""} {
	mged_update 0
    }

    return $mged_gui($id,mgc_comb)
}

proc mouse_solid_edit_select { x y } {
    global mged_players
    global mged_gui

    set spath [mouse_get_spath $x $y]
    if {$spath == ""} {
	return
    }

    _mged_press sill
    _mged_ill $spath

    mged_apply_all "set mouse_behavior d"
    foreach id $mged_players {
	set mged_gui($id,mouse_behavior) d
    }
}

proc mouse_matrix_edit_select { x y } {
    global mged_players
    global mged_gui

    set spath_and_pos [mouse_get_spath_and_pos $x $y]
    if {[llength $spath_and_pos] != 2} {
	return
    }

    _mged_press oill
    _mged_ill [lindex $spath_and_pos 0]
    _mged_matpick [lindex $spath_and_pos 1]

    mged_apply_all "set mouse_behavior d"
    foreach id $mged_players {
	set mged_gui($id,mouse_behavior) d
    }
}

proc mouse_comb_edit_select { x y } {
    global mged_players
    global mged_gui
    global comb_control

    set comb [mouse_get_comb $x $y]
    if {$comb == ""} {
	return
    }

    set win [winset]
    set id [get_player_id_dm $win]
    init_comb $id
    set comb_control($id,name) $comb
    comb_reset $id

    mged_apply_all "set mouse_behavior d"
    foreach id $mged_players {
	set mged_gui($id,mouse_behavior) d
    }
}
