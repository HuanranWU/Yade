with (import <nixpkgs> {});

( let
minieigen = pkgs.python37Packages.buildPythonPackage rec{
      name = "minieigen";

      src = pkgs.fetchFromGitHub {
        owner = "eudoxos";
        repo = "minieigen";
        rev = "7bd0a2e823333477a2172b428a3801d9cae0800f";
        sha256 = "1jksrbbcxshxx8iqpxkc1y0v091hwji9xvz9w963gjpan4jf61wj";
      };

      buildInputs = [ unzip python37Packages.boost eigen ];

      patchPhase = ''
        sed -i "s/^.*libraries=libraries.//g" setup.py 
      '';

      preConfigure = ''
        export LDFLAGS="-L${eigen.out}/lib -L${python37Packages.boost.out} -lboost_python37"
        export CFLAGS="-I${eigen.out}/include/eigen3"
      '';
};

in 

{ yade-env = pkgs.python37.buildEnv.override rec{

        extraLibs = with pkgs.python37Packages;[
                        pygments mpi4py pexpect decorator numpy xlib
                        ipython ipython_genutils traitlets pygraphviz
                        six minieigen ipython future matplotlib tkinter pkgs.cmake
                      ] ;
        ignoreCollisions = true;
    };
}
)
