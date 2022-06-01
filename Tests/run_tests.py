import os
import re
import shutil
import time
import subprocess

TEST_GAMEBUILDS = []
TEST_FILE = 'testoutput.log'

def parse_args(filepath):
    args = {
        'b5135': None,
        'steampipe': None
    }
    with open(filepath, 'r') as file:
        for line in file:
            line_args = line.split('=')
            if len(line_args) < 2:
                print(line, 'is not a valid config line')
            else:
                build = line_args[0].strip()
                path = line_args[1].strip()
                args[build] = path
    return args

def copy_all(src, dst):
    """Copy all files from src to dst"""
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            if not os.path.isdir(d):
                os.mkdir(d)
            copy_all(s,d)
        if os.path.isfile(s):
            shutil.copyfile(s,d)
       
class GameBuild:
    def __init__(self, game, build, test_path):
        self.test_path = test_path
        self.game = game
        self.build = build
        self.path = None
        self.game_path = None
        self.test_output = None
        self.output_filename = None
        self.executable = None
        self.spt_path = None
        self.spt_name = None
        self.spt_out = None

    def __str__(self):
        return "%s - %s - %s - %s" % (self.game, self.build, self.path, self.test_path)

    def start_game(self):
        """Start the game."""
        exec_path = os.path.join(self.path, self.executable)
        cmd = "%s -game \"%s\" -w 640 -h 480 -window -novid -console +volume 0 +plugin_load %s +tas_test_automated_validate \"test %s\"" % (exec_path, self.game, self.spt_name, self.output_filename)
        subprocess.run(cmd)

    def set_path(self, path, executable, spt_path, spt_name):
        """Set path and some other info."""
        game_dir = os.path.join(path, self.game)
        if os.path.isdir(game_dir): # Check if game is installed
            self.game_path = game_dir
            self.path = path
            self.output_filename = self.game.replace(' ', '_') + "-" + TEST_FILE
            self.test_output = os.path.join(self.path, self.output_filename)
            self.executable = executable
            self.spt_path = spt_path
            self.spt_out = os.path.join(game_dir, spt_name + ".dll")
            self.spt_name = spt_name

    def copy_files(self):
        """Copy the test files to the game"""
        copy_all(self.test_path, self.game_path)
        shutil.copyfile(self.spt_path, self.spt_out)

    def print_test_output(self):
        """Print the test output"""
        print("[TEST] Game: %s, build %s" % (self.game, self.build))
        if not os.path.isfile(self.test_output):
            print("[TEST] Output file %s does not exist. Game likely crashed." % self.test_output)
        else:
            with open(self.test_output) as fp:
                for line in fp:
                    print(line, end='')

    def run_test(self):
        """Run test"""
        self.copy_files()
        if os.path.isfile(self.test_output):
            os.remove(self.test_output)
        self.start_game()

def get_game(name):
    """Get gamebuild given the folder name or arg name"""
    result = name.split('-')
    game = result[0].strip()
    build = result[1].strip()
    return GameBuild(game, build, name)

def find_tests():
    """Finds all games with tests based on the folders inside this folder"""
    for dir_name in os.listdir():
        if os.path.isdir(dir_name):
            game = get_game(dir_name)
            TEST_GAMEBUILDS.append(game)

def add_build_path(build_number, build_path, build_executable, spt_path, spt_name):
    """Add information about the build"""
    for build in TEST_GAMEBUILDS:
        if build.build == build_number:
            build.set_path(build_path, build_executable, spt_path, spt_name)

def find_paths():
    """Look up paths for different game builds from the config file."""
    args = parse_args('config.ini')
    if args['b5135'] is not None:
        add_build_path("5135", args['b5135'], "hl2.exe", get_2007_spt(), "spt") 
    if args['steampipe'] is not None:
        add_build_path("steampipe", args['steampipe'], "hl2.exe", get_2013_spt(), "spt-2013")

    for build in TEST_GAMEBUILDS:
        if build.path:
            print("Build %s for %s located in %s" % (build.build, build.game, build.path))
        else:
            print("Build %s for %s is not installed." % (build.build, build.game))

def get_2007_spt():
    releases = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'Release\\spt.dll'))
    return releases
    
def get_2013_spt():
    releases = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'Release 2013\\spt-2013.dll'))
    return releases

def run_tests():
    """Run all tests for games that are installed and configured."""
    for build in TEST_GAMEBUILDS:
        if build.path:
            print("Running test for game %s, build %s" % (build.game, build.build))
            build.run_test()
        else:
            print("Skipping test for game %s, build %s" % (build.game, build.build))
   
def collect_test_results():
    """Collects test results from all the output files and prints them."""
    for build in TEST_GAMEBUILDS:
        if build.path:
            build.print_test_output()

find_tests()
find_paths()
run_tests()
collect_test_results()
