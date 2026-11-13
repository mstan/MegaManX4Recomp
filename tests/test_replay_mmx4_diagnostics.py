"""Diagnostic baseline preparation stays isolated and rejects incomplete bundles."""
import argparse,importlib.util,json,os,tempfile,unittest
from pathlib import Path

root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('replay',root/'tools/replay_mmx4_diagnostics.py')
replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)

class Preparation(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.session=self.root/'session';self.session.mkdir()
        (self.session/'inputs.csv').write_text('# mmx4-input-v2\n# execution e\n# plan p\n# build b\nN 0\nC 0 1 65535\nN 1\n')
        (self.session/'card1.mcd').write_bytes(bytes(131072))
        (self.session/'card2.absent').touch()
        (self.session/'mods-state.toml').write_text('format_version = 2\n')
        self.exe=self.root/'game.exe';self.exe.touch()
        self.disc=self.root/'disc.cue';self.disc.touch()
        self.game=self.root/'game.toml'
        self.game.write_text('[game]\nexe = "SLUS_005.61"\ndisc = "disc.cue"\nentry_pc = "0x800DAE8C"\n')
        self.args=argparse.Namespace(recording=self.session,exe=self.exe,disc=self.disc,game=self.game,settings=None,output=self.root/'replay')
    def metadata(self,value):
        (self.session/'session.json').write_text(json.dumps(value))
    def test_restore_segment_has_private_baseline_and_original_bundle_is_untouched(self):
        self.metadata({'netplay':False,'baseline':'baseline.pst'})
        original=b'validated-runtime-state';(self.session/'baseline.pst').write_bytes(original)
        output,_,trace=replay.prepare(self.args)
        for token in ('openbios','scph1001'):
            self.assertEqual((output/'saves'/token/'state_800DAE8C_slot00.pst').read_bytes(),original)
        self.assertEqual((self.session/'baseline.pst').read_bytes(),original)
        self.assertEqual((output/'diagnostic-baseline.pst').read_bytes(),original)
        self.assertEqual(trace.read_text().count('N '),1)
    def test_missing_baseline_is_rejected_before_creating_output(self):
        self.metadata({'baseline':'baseline.pst'})
        with self.assertRaisesRegex(ValueError,'baseline'):replay.prepare(self.args)
        self.assertFalse(self.args.output.exists())
    def test_baseline_path_cannot_escape_bundle(self):
        self.metadata({'baseline':'../outside.pst'})
        with self.assertRaisesRegex(ValueError,'filename'):replay.prepare(self.args)
        self.assertFalse(self.args.output.exists())
    def test_original_input_only_sessions_need_no_slot_restore(self):
        self.metadata({'netplay':False})
        output,_,_=replay.prepare(self.args)
        self.assertFalse((output/'diagnostic-baseline.pst').exists())

    def test_bundled_bios_is_copied_for_release_startup_verification(self):
        bios=self.root/'bios';bios.mkdir()
        (bios/'openbios.bin').write_bytes(b'bundled image')
        (bios/'OpenBIOS.LICENSE').write_text('redistribution notice')
        output,_,_=replay.prepare(self.args)
        self.assertEqual((output/'bios/openbios.bin').read_bytes(),b'bundled image')
        self.assertEqual((output/'bios/OpenBIOS.LICENSE').read_text(),'redistribution notice')
        (bios/'openbios.bin').write_bytes(b'changed source')
        self.assertEqual((output/'bios/openbios.bin').read_bytes(),b'bundled image')
    def test_network_bundle_does_not_launch_as_offline_replay(self):
        self.metadata({'netplay':True,'baseline':'baseline.pst'})
        with self.assertRaisesRegex(ValueError,'Netplay'):replay.prepare(self.args)
        self.assertFalse(self.args.output.exists())

    def test_bundled_native_toolchain_is_copied_without_using_the_installation(self):
        compiler=self.root/'overlay_toolchain/tcc/tcc.exe';compiler.parent.mkdir(parents=True)
        compiler.write_bytes(b'packaged native compiler')
        header=self.root/'overlay_toolchain/include/psx_runtime.h';header.parent.mkdir()
        header.write_bytes(b'packaged ABI')
        output,_,_=replay.prepare(self.args)
        self.assertEqual((output/'overlay_toolchain/tcc/tcc.exe').read_bytes(),compiler.read_bytes())
        self.assertEqual((output/'overlay_toolchain/include/psx_runtime.h').read_bytes(),header.read_bytes())
        compiler.write_bytes(b'changed installation')
        self.assertEqual((output/'overlay_toolchain/tcc/tcc.exe').read_bytes(),b'packaged native compiler')

    def test_appimage_container_is_rejected_before_any_output_is_created(self):
        self.metadata({'netplay':False})
        # Detect a renamed type-2 AppImage as well as the conventional suffix.
        self.exe.write_bytes(b'\x7fELF\x02\x01\x01\x00AI\x02')
        with self.assertRaisesRegex(ValueError,'--appimage-extract.*--exe.*--game'):
            replay.prepare(self.args)
        self.assertFalse(self.args.output.exists())
        self.exe.write_bytes(b'')
        self.args.exe=self.exe.with_suffix('.AppImage');self.args.exe.touch()
        with self.assertRaisesRegex(ValueError,'AppRun'):replay.prepare(self.args)
        self.assertFalse(self.args.output.exists())

    def test_extracted_linux_runtime_uses_copied_payload_and_recorded_choices(self):
        self.metadata({'netplay':False})
        image=self.root/'squashfs-root/usr'
        binary=image/'bin/MegaManX4Recomp';binary.parent.mkdir(parents=True)
        binary.write_bytes(b'\x7fELF\x02\x01\x01\x00\x00\x00\x00')
        payload=image/'share/megamanx4recomp';payload.mkdir(parents=True)
        game=payload/'game.toml';game.write_text(self.game.read_text())
        (payload/'mods').mkdir();(payload/'mods/state.toml').write_text('installation choices')
        (payload/'mods/bundled').mkdir();(payload/'mods/bundled/package').write_text('bundled mod')
        (payload/'assets').mkdir();(payload/'assets/icon.png').write_bytes(b'icon')
        (self.session/'settings.toml').write_text('[video]\nvsync = false\n[memcard]\ndir = "personal"\n')
        (image/'lib').mkdir();(image/'lib/libSDL2.so.0').write_bytes(b'packaged library')
        (binary.parent/'libadjacent.so.1').write_bytes(b'adjacent library')
        self.args.exe=binary;self.args.game=game
        output,target,_=replay.prepare(self.args)
        self.assertEqual(target.parent,output)
        self.assertEqual((output/'mods/state.toml').read_bytes(),(self.session/'mods-state.toml').read_bytes())
        self.assertEqual((payload/'mods/state.toml').read_text(),'installation choices')
        self.assertEqual((output/'mods/bundled/package').read_text(),'bundled mod')
        self.assertEqual((output/'assets/icon.png').read_bytes(),b'icon')
        self.assertEqual((output/'lib/libSDL2.so.0').read_bytes(),b'packaged library')
        self.assertEqual((output/'lib/libadjacent.so.1').read_bytes(),b'adjacent library')
        settings=(output/'settings.toml').read_text()
        self.assertIn('vsync = false',settings)
        self.assertNotIn('personal',settings)
        (image/'lib/libSDL2.so.0').write_bytes(b'changed original')
        self.assertEqual((output/'lib/libSDL2.so.0').read_bytes(),b'packaged library')

    def test_replay_environment_uses_private_roots_and_no_inherited_mount(self):
        output=self.root/'private';output.mkdir()
        (output/'diagnostic-baseline.pst').write_bytes(b'baseline')
        original={'APPIMAGE':'/personal/game.AppImage','APPDIR':'/tmp/.mount-personal',
            'RECOMP_APPIMAGE_PATH':'/personal/game.AppImage','MMX4_RECOMP_APPIMAGE_PATH':'/personal/game.AppImage',
            'MMX4_RECOMP_DATA_DIR':'/personal/data','MMX4_RECOMP_SEED_ONLY':'1',
            'LD_LIBRARY_PATH':'/personal/lib','PSX_LOAD_SLOT':'7','PSX_SPEED':'2',
            'MMX4_DIAGNOSTIC_REPLAY':'/personal/inputs.csv','PATH':'system path'}
        trace=output/'replay-inputs.csv'
        env=replay.replay_environment(output,trace,original)
        self.assertEqual(env['MMX4_RECOMP_DATA_DIR'],str(output))
        self.assertEqual(env['MMX4_DIAGNOSTIC_REPLAY'],str(trace))
        self.assertEqual(env['LD_LIBRARY_PATH'],os.pathsep.join((str(output/'lib'),str(output))))
        self.assertEqual(env['PSX_LOAD_SLOT'],'0')
        self.assertEqual(env['PATH'],'system path')
        for key in ('APPIMAGE','APPDIR','RECOMP_APPIMAGE_PATH','MMX4_RECOMP_APPIMAGE_PATH','MMX4_RECOMP_SEED_ONLY','PSX_SPEED'):
            self.assertNotIn(key,env)
        self.assertEqual(original['MMX4_RECOMP_DATA_DIR'],'/personal/data')

if __name__=='__main__':unittest.main()
