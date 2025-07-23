import re
import subprocess

findgtkversion = re.compile(r'Init from *\[dueca-(gtk[0-9]+)\]')

def which_dueca_gtk():

    try:
        res = subprocess.run(('./dueca_run.x', '--version'), stdout=subprocess.PIPE)
        if res.returncode:
            raise RuntimeError(f"Cannot run dueca_run.x to get version info: {res.returncode}")

        for l in res.stdout.decode('utf-8').split('\n'):
            m = findgtkversion.match(l)
            if m:
                return m.group(1)

        return "none"
    except FileNotFoundError:
        raise RuntimeError("No dueca_run.x executable")


if __name__ == '__main__':
    print(which_dueca_gtk())
