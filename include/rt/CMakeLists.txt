set(rt_headers
  anim.h
  application.h
  arb_edit.h
  binunif.h
  boolweave.h
  calc.h
  cmd.h
  comb.h
  db4.h
  db5.h
  db_attr.h
  db_attr.h
  db_diff.h
  db_fullpath.h
  db_instance.h
  db_internal.h
  db_io.h
  debug.h
  defines.h
  directory.h
  dspline.h
  func.h
  functab.h
  geom.h
  global.h
  hit.h
  htbl.h
  mater.h
  misc.h
  mem.h
  nmg.h
  nongeom.h
  nurb.h
  op.h
  overlap.h
  pattern.h
  piece.h
  prep.h
  primitives/arb8.h
  primitives/ars.h
  primitives/bot.h
  primitives/brep.h
  primitives/cline.h
  primitives/dsp.h
  primitives/ell.h
  primitives/epa.h
  primitives/hf.h
  primitives/metaball.h
  primitives/pg.h
  primitives/pipe.h
  primitives/rhc.h
  primitives/rpc.h
  primitives/sketch.h
  primitives/tgc.h
  primitives/tor.h
  private.h
  ray_partition.h
  region.h
  resource.h
  rt_instance.h
  rt_tcl.h
  search.h
  seg.h
  shoot.h
  solid.h
  soltab.h
  space_partition.h
  tie.h
  timer.h
  tol.h
  tree.h
  version.h
  view.h
  vlist.h
  wdb.h
  xray.h
  )

install(FILES ${rt_headers} DESTINATION ${INCLUDE_DIR}/brlcad/rt)

CMAKEFILES(${rt_headers})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
