##############################################################################
# Author(s): Christopher J. Havlicek
##############################################################################

import sys
import os
import shutil
import subprocess
import time
import argparse

# TODO: check all exit codes from subprocess executions.

timeout = "0"
master_port = '10055'

crete_dispatch_config = {
'path' : 'crete.dispatch.xml',
'content' :
'''<crete>
	<mode>distributed</mode>
	<vm>
		<image>
			<path>/opt/crete/images/ubuntu-14-x64.img</path>
			<update>true</update>
		</image>
		<arch>x64</arch>
		<snapshot>sudo_test</snapshot>
		<args>-m 512 -nographic -L /opt/crete/etc/pc-bios</args>
	</vm>
	<svm>
		<args>
			<symbolic>
				--max-memory=1000
				--disable-inlining
				--use-forked-solver
				--max-sym-array-size=4096
				--max-instruction-time=10.
				--max-time=360.
				-randomize-fork=false
				-search=dfs
			</symbolic>
		</args>
	</svm>
	<profile>
		<interval>1000</interval>
	</profile>
</crete>'''
}
crete_sanity_test = {
'path' : 'crete_sanity_test.cpp',
'content' :
'''#include <iostream>
#include <fstream>

int main( int argc, char* argv[] )
{
    if( argc != 2 )
    {
        std::cerr << "missing arg[1] == filename" << std::endl;
	return 1;
    }

    std::ifstream ifs( argv[1] );

    if( !ifs.good() )
    {
        std::cerr << "unable to open: " << argv[1] << std::endl;
	return 1;
    }

    char c;

    ifs >> c;

    if     ( c == \'x\' )
        std::cout << "c == " << c << std::endl;
    else if( c == \'y\' )
        std::cout << "c == " << c << std::endl;
    else if( c == \'z\' )
        std::cout << "c == " << c << std::endl;

    return 0;
}'''
}
crete_sanity_guest_config = {
'path' : 'crete.guest.xml',
'content' :
'''<crete>
  <exec>/home/test/excite/test/harness</exec>
  <args>
    <arg concolic="false" value="/home/test/excite/test/input.bin" size="" index="1"/>
  </args>
  <files>
    <file concolic="true" size="1" argv_index="1" path="/home/test/excite/test/input.bin"/>
  </files>
</crete>'''
}
crete_env_file = {
'path' : 'env.txt',
'content' :
    { 'LD_LIBRARY_PATH' : '' }
}
crete_guest_config_name = 'crete.guest.xml'
crete_test_archive_unzipped_dir = 'test'
crete_test_archive_zipped_file = 'test.zip'

def prep_env():
    if os.path.exists( crete_test_archive_unzipped_dir ):
        shutil.rmtree( crete_test_archive_unzipped_dir )

    os.makedirs( crete_test_archive_unzipped_dir )
        
    if not os.path.exists( crete_dispatch_config['path'] ):
        with open( crete_dispatch_config['path'], 'w' ) as f:
            f.write( crete_dispatch_config['content'] )

def run_sanity_test():
    prep_env()

    prev_cwd = os.getcwd()

    os.chdir( crete_test_archive_unzipped_dir )

    with open( crete_sanity_test['path'], 'w' ) as f:
        f.write( crete_sanity_test['content'] )
    with open( crete_sanity_guest_config['path'], 'w' ) as f:
        f.write( crete_sanity_guest_config['content'] )

    cmd = [ 'g++', crete_sanity_test['path'], '-o', 'harness' ]

    subprocess.call( cmd )

    os.chdir( prev_cwd )

    zip_test_archive()
    run_crete()

    return verify_sanity_test_results()

def verify_sanity_test_results():
    # TODO: should perform more rigorous test e.g., verifying contents of TC, or output check.
    
    # Check if there are the expected number of TCs generated.
    return 4 == len( os.listdir( 'dispatch/last/crete.guest.xml/test-case' ) )

def zip_test_archive():
    # TODO: maybe use the builtin zip lib
    if os.path.exists( crete_test_archive_zipped_file ):
        os.remove( crete_test_archive_zipped_file )

    cmd = [ 'zip', '-r', crete_test_archive_zipped_file, crete_test_archive_unzipped_dir ]
    subprocess.call( cmd )


def run_crete():
    cmd_dispatch = [ 'crete-dispatch'
                   , '--config=%s' % ( crete_dispatch_config['path'] )
                   , '--port=%s' % ( master_port )
                   , '--archive=%s' % ( crete_test_archive_zipped_file )
                   , '--item=%s' % ( os.path.join(
                       crete_test_archive_unzipped_dir, crete_guest_config_name ) ) 
                   , '--time-out=%s' % ( timeout ) ]
    cmd_vm_node =  [ 'crete-vm-node'
                   , '--ip=%s' % ( 'localhost' )
                   , '--port-master=%s' % ( master_port )
                   , '--instances=%s' % ( '1' ) ]
    cmd_svm_node = [ 'crete-svm-node'
                   , '--ip=%s' % ( 'localhost' )
                   , '--port=%s' % ( master_port )
                   , '--instances=%s' % ( '1' ) ]
        
    child_dispatch = subprocess.Popen( cmd_dispatch )
    time.sleep( 2 )
    child_vm_node = subprocess.Popen( cmd_vm_node )
    time.sleep( 2 )
    child_svm_node = subprocess.Popen( cmd_svm_node )

    runtime = 0
    finished = False

    children = [ child_dispatch, child_vm_node, child_svm_node ]

    while not finished and int( timeout ) > runtime:
        for child in children:
            if child.poll() is not None:
                finished = True
        time.sleep( 1 )
        runtime += 1

    time.sleep( 5 ) # Give CRETE a few to finalize.

    for child in children:
        # Kill child if it is still alive after timeout.
        if child.poll() is None:
            print( 'Warning: CRETE failed to terminate. '
                   'Killing: %d' % ( child.pid ) )
            sys.stdout.flush()
            child.terminate()
        # Reap the child process.
        child.wait()

    with open( 'dispatch/last/crete.guest.xml/log/finish.log', 'r' ) as flog:
        print( 'Final status:\n %s' % ( flog.read() ) )
        sys.stdout.flush()

def run_test( target_dir ):
    print( 'Testing:\n\tTarget dir: %s'
           % ( target_dir ) )

    prep_env()

    prep_test_dir( target_dir )

    zip_test_archive()

    run_crete()

    process_crete_results()

def process_crete_results():
    # TODO: analyze output of CRETE e.g., target_execution.log
    pass

def prep_test_dir( dir_path ):
    if os.path.exists( crete_test_archive_unzipped_dir ):
        shutil.rmtree( crete_test_archive_unzipped_dir )

    shutil.copytree( dir_path, crete_test_archive_unzipped_dir )
    
def parse_args():
    # TODO: should verify that input parameters are sane e.g., memdump is a
    # file, memstart is a valid hex (0x) number.
    parser = argparse.ArgumentParser()

    parser.add_argument( '-s'
                       , '--sanity_check'
                       , default = False
                       , help = 'Run sanity check before test.' )
    parser.add_argument( '-d'
                       , '--crete_dir'
                       , default = ''
                       , help = 'CRETE Binary build dir. Defaults to one in'
                         'PATH' )
    parser.add_argument( '-a'
                       , '--test_archive_dir'
                       , help = 'Directory with harness and config to test' )
    parser.add_argument( '-t'
                       , '--timeout'
                       # , required = True
                       # , type = int
                       , help = 'Time duration to test each entry point.' )
    parser.add_argument( '-r'
                       , '--replay'
                       , default = None
                       , help = 'arg:[exe] Replay all test cases on the host after'
                         ' testing.' )
    parser.add_argument( '-g'
                       , '--gen_coverage'
                       , default = None
                       , help = 'arg:[dir] Generate code coverage for provided directory' )

    args = parser.parse_args()

    global timeout
    timeout = args.timeout

    return args

def init_crete_cmds( bin_dir ):
    if not os.path.exists( os.path.join( bin_dir, 'crete-dispatch' ) ):
        print( 'Failed to find crete-dispatch: %s' % (crete_dispatch_cmd) )
        print( 'Aborting!!!' )
        sys.exit( 1 )


    os.environ['PATH'] = os.environ['PATH'] + os.pathsep + bin_dir
    print( os.environ['PATH'] )
    os.environ['LD_LIBRARY_PATH'] = bin_dir \
                                  + os.pathsep \
                                  + os.path.join( bin_dir, 'boost' )

def replay_test_cases( exe ):
    if not os.path.exists( 'dispatch/last/crete.guest.xml/test-case-parsed' ):
        cmd = [ 'crete-tc-compare'
              , '--batch-patch'
              , 'dispatch/last' ]

        print( cmd )
        subprocess.call( cmd )

    prev_cwd = os.getcwd()
    os.chdir( 'dispatch/last/crete.guest.xml' )

    if os.path.exists( 'replay' ):
        shutil.rmtree( 'replay' )
    os.mkdir( 'replay' )
    os.chdir( 'replay' )


    with open( crete_env_file['path'], 'w' ) as f:
        for k,v in crete_env_file['content'].iteritems():
            f.write( '%s %s' % ( k, v ) )

    cmd = [ 'crete-tc-replay'
          , '--exec'
          , exe
          , '--tc-dir'
          , '../test-case-parsed'
          , '--config'
          , '../guest-data/crete-guest-config.serialized'
          , '-v'
          , crete_env_file['path']
          , '--log' ]

    print( cmd )
    subprocess.call( cmd )

    os.chdir( prev_cwd )

def generate_coverage( source_dir ):
    prev_cwd = os.getcwd()
    os.chdir( 'dispatch/last/crete.guest.xml/replay' )

    cmd = [ 'lcov'
          , '--base-directory'
          , source_dir
          , '--directory'
          , source_dir
          , '-c'
          , '-o'
          , 'test.info' ]

    print( cmd )
    subprocess.call( cmd )

    cmd = [ 'genhtml'
          , '-o'
          , 'html'
          , '-t'
          , 'Test Coverage'
          , 'test.info' ]

    print( cmd )
    subprocess.call( cmd )

    os.chdir( prev_cwd )

def process_args( args ):
    init_crete_cmds( os.path.realpath( os.path.expanduser( args.crete_dir ) ) )

    if args.sanity_check:
        if not run_sanity_test():
            print( 'Sanity check failed!' )
            print( 'Aborting!!!' )
            sys.exit( 1 )

        print( 'Sanity check passed' )

        return

    if args.test_archive_dir:
        if not os.path.exists( 
            os.path.join( args.test_archive_dir,
                          crete_guest_config_name ) ):
                print( 'Test archive config not found!' )
                print( 'Aborting!!!' )
                sys.exit( 1 )
        run_test( args.test_archive_dir )

    if args.replay:
        crete_env_file[ 'content' ] \
                [ 'LD_LIBRARY_PATH' ] = os.path.abspath( os.path.realpath(
                    args.crete_dir ) )
        replay_test_cases( os.path.abspath( args.replay ) )
    if args.gen_coverage:
        if os.path.exists( 'dispatch/last/crete.guest.xml/replay' ):
            crete_env_file[ 'content' ] \
                    [ 'LD_LIBRARY_PATH' ] = args.crete_dir
            generate_coverage( os.path.abspath( args.gen_coverage ) )
        else:
            print( 'Failed to generate coverage: no replay found.' )


if __name__ == '__main__':
    args = parse_args()
    process_args( args )
    # TODO: ensure QEMU is terminated.
    # Hack to ensure QEMU has been terminated
    subprocess.call( ['pkill', 'crete-qemu'] )


