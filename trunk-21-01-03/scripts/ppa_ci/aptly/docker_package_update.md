

aptly repo create -distribution=buster -component=main yadedaily-buster && aptly repo add yadedaily-buster *.deb && aptly repo add yadedaily-buster *.dsc&& AWS_ACCESS_KEY_ID="XXX"  AWS_SECRET_ACCESS_KEY="XXX" aptly publish repo yadedaily-buster s3:yadedaily-repo:/debian/

aptly repo create -distribution=buster -component=main yadedaily-buster
aptly repo add yadedaily-buster *.deb
aptly repo add yadedaily-buster *.dsc
AWS_ACCESS_KEY_ID="XXX"  AWS_SECRET_ACCESS_KEY="XXX" aptly publish repo yadedaily-buster s3:yadedaily-repo:/debian/