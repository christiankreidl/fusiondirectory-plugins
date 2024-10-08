<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2020  FusionDirectory
  Copyright (C) 2020  Clement Oudot <clement.oudot@worteks.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*!
  \brief   NextCloud plugin

  This class provides the functionality to read and write all attributes
  relevant for NextCloud accounts from/to the LDAP.
 */
class nextcloudAccount extends simplePlugin
{
  protected $displayHeader  = TRUE;

  static function plInfo (): array
  {
    return [
      'plShortName'     => _('NextCloud'),
      'plDescription'   => _('NextCloud settings'),
      'plIcon'          => 'geticon.php?context=applications&icon=nextcloud&size=48',
      'plSmallIcon'     => 'geticon.php?context=applications&icon=nextcloud&size=16',
      'plSelfModify'    => TRUE,
      'plObjectClass'   => ['fdNextCloudAccount'],
      'plObjectType'    => ['user'],
      'plProvidedAcls'  => parent::generatePlProvidedAcls(static::getAttributesInfo()),
    ];
  }

  /*!
  *  \brief The main function : information about attributes
  */
  static function getAttributesInfo (): array
  {
    return  [
      'main' => [
        'name'  => _('NextCloud account'),
        'attrs' => [
          new BytesSizeAttribute(
            _('File quota'), _('Define quota size'),
            'fdNextCloudQuota', FALSE
          ),
          new PathAttribute(
            _('Home folder'), _('Define the home folder'),
            'fdNextCloudHomeFolder', FALSE
          )
        ]
      ]
    ];
  }
}
