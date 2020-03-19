#!/bin/bash

echo "-----------------------------------------------------------------"
echo "Starting the installation (or update) of p4d deamon"
echo "   Deamon to fetch sensor data of the 'Lambdatronic s3200' and store to a MySQL database"
echo "-----------------------------------------------------------------"

echo -n "Continue? [y/N] "
echo ""
read -n 1 c

if [ "${c}" <> "y" ]; then
    exit 0
fi

IP=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')

apt update || exit 1
apt -y dist-upgrade || exit 1
apt -y install libssl1.1 libcurl4  libxml2 openssl libmariadb3 || exit 1
apt -y install apache2 libapache2-mod-php php7.2-mysql python-mysql.connector php-gd php-mysql php-mbstring || exit 1
apt -y install mariadb-server || exit 1

wget www.jwendel.de/p4d/p4d-latest.deb -P /tmp || exit 1
dpkg --install /tmp/p4d-latest.deb || exit 1

systemctl daemon-reload || exit 1
systemctl enable p4d || exit 1

echo "alias p4db='mysql -u p4 -D p4 -pp4'" >> ~/.bashrc
echo "alias vs='tail -f /var/log/syslog'" >> ~/.bashrc
echo "alias va='tail -f /var/log/apache2/error.log'" >> ~/.bashrc

echo "-----------------------------------------------------------------"
echo "Installation completed, you can reach the web interface at http://<raspi-ip>/p4"
echo "I guess youre IP is ${IP}, then use: http://${IP}/p4"
echo ""
echo "Added aliases for convenience:"
echo "p4dp  - go to the SQL prompt"
echo "vs    - view syslog (abort with CTRL-C)"
echo "va    - view apace error log (abort with CTRL-C)"
echo "-----------------------------------------------------------------"

echo "Reboot now!"
