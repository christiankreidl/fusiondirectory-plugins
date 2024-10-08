<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2007  Fabian Hicker
  Copyright (C) 2011-2018  FusionDirectory

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

class MailQuotaAttribute extends UnitIntAttribute
{
  private $quotaUsage;

  function __construct ($label, $description, $ldapName, $required, $min = FALSE, $max = FALSE, $defaultValue = "", $acl = "")
  {
    $units = [
      1         => _('MiB'),
      1024 ** 1 => _('GiB'),
      1024 ** 2 => _('TiB'),
    ];
    parent::__construct($label, $description, $ldapName, $required, $units, $min, $max, $defaultValue, $acl);
  }

  function setMailMethod ($mailMethod)
  {
    /* Read quota */
    $this->setValue((string)$mailMethod->getQuota($this->getValue()));
    $this->quotaUsage = $mailMethod->getQuotaUsage();
    if ($mailMethod->is_error()) {
      $error = new SimplePluginError(
        $this,
        htmlescape(sprintf(
                     _('Cannot read quota settings: %s'),
                     $mailMethod->get_error()
                   ))
      );
      $error->display();
    }
  }

  function renderAttribute (array &$attributes, bool $readOnly, bool $readable, bool $writable)
  {
    parent::renderAttribute($attributes, $readOnly, $readable, $writable);
    $attributes[$this->getLdapName() . '_usage'] = [
      'htmlid'       => $this->getHtmlId() . '_usage',
      'label'        => '{literal}' . _('Quota usage') . '{/literal}',
      'description'  => _('Part of the quota which is used'),
      'input'        => mailMethod::quota_to_image($this->quotaUsage, $this->getValue()),
      'subattribute' => FALSE,
      'required'     => FALSE,
      'readable'     => $readable,
      'writable'     => $writable,
    ];
  }
}

/*!
  \brief   mail plugin

  This class provides the functionality to read and write all attributes
  relevant for mail accounts from/to the LDAP.

  It does syntax checking and displays the formulars required.
  Special handling like sieve or imap actions will be implemented
  by the mailMethods.

 */

class mailAccount extends simplePlugin
{
  protected $displayHeader = TRUE;

  private $mainSectionAttrs = [];

  public $mailMethod        = NULL;
  public $initialMailMethod = NULL;

  static function plInfo (): array
  {
    return [
      'plShortName'    => _('Mail'),
      'plDescription'  => _('Mail settings'),
      'plFilter'       => '(mail=*)',
      'plIcon'         => 'geticon.php?context=applications&icon=internet-mail&size=48',
      'plSmallIcon'    => 'geticon.php?context=applications&icon=internet-mail&size=16',
      'plSelfModify'   => TRUE,
      'plPriority'     => 4,
      'plObjectClass'  => ['gosaMailAccount'],
      'plObjectType'   => ['user'],
      'plProvidedAcls' => parent::generatePlProvidedAcls(static::getAttributesInfo()),
      'plForeignKeys'  => [
        'gosaMailServer' => [
          ['serverGeneric', 'cn'],
        ]
      ]
    ];
  }

  /*!
  *  \brief The main function : information about attributes
  */
  static function getAttributesInfo (): array
  {
    return [
      'main'            => [
        'name'  => _('Mail account'),
        'attrs' => [
          new MailAttribute(
            _('Primary address'), _('Primary mail address'),
            'mail', TRUE
          ),
          new SelectAttribute(
            _('Server'), _('Specify the mail server where the user will be hosted on'),
            'gosaMailServer', FALSE,
            []
          ),
          new MailQuotaAttribute(
            _('Quota size'), _('Define quota size in MiB'),
            'gosaMailQuota', FALSE,
            0, FALSE
          )
        ]
      ],
      'other_addresses' => [
        'name'  => _('Other addresses and redirections'),
        'attrs' => [
          new SetAttribute(
            new MailAttribute(
              _('Alternative addresses'), _('List of alternative mail addresses'),
              'gosaMailAlternateAddress'
            )
          ),
          new MailsAttribute(
            _('Forward messages to'), _('Addresses to which messages should be forwarded'),
            'gosaMailForwardingAddress'
          )
        ]
      ],
      'options1'        => [
        'name'  => _('Vacation message'),
        'attrs' => [
          new BooleanAttribute(
            _('Activate vacation message'),
            _('Select to automatically response with the vacation message defined below'),
            'flag_vacation', FALSE, FALSE, '',
            'V', ''
          ),
          new DateAttribute(
            _('from'), 'Starting date for vacation message',
            'gosaVacationStart', FALSE,
            'U'
          ),
          new DateAttribute(
            _('till'), 'Ending date for vacation message',
            'gosaVacationStop', FALSE,
            'U'
          ),
          new TextAreaAttribute(
            _('Vacation message'), 'The message you wish be sent during your absence',
            'gosaVacationMessage'
          ),
        ]
      ],
      'advanced'        => [
        'name'  => _('Advanced mail options'),
        'attrs' => [
          new FlagsAttribute('gosaMailDeliveryMode', [
            'flag_ownmailbox',
            'flag_vacation',
            'flag_localonly'
          ]),
          new BooleanAttribute(
            _('User is only allowed to send and receive local mails'),
            _('Select if user can only send and receive inside his own domain'),
            'flag_localonly', FALSE, FALSE, '',
            'L', ''
          ),
          new BooleanAttribute(
            _('No delivery to own mailbox'),
            _('Select if you want to forward mails without getting own copies of them'),
            'flag_ownmailbox', FALSE, FALSE, '',
            'I', ''
          ),
        ]
      ]
    ];
  }

  function __construct ($dn = NULL, $object = NULL, $parent = NULL, $mainTab = FALSE)
  {
    global $config;
    parent::__construct($dn, $object, $parent, $mainTab);

    $this->mainSectionAttrs = array_keys($this->attributesInfo['main']['attrs']);

    $this->attributesAccess['gosaMailServer']->setChoices(array_keys(mailMethod::getMailServers()));
    $this->attributesAccess['gosaMailServer']->setSubmitForm('mailServerChanged');
    $this->mailServerChanged();

    $this->attributesAccess['flag_ownmailbox']->setInLdap(FALSE);
    $this->attributesAccess['flag_vacation']->setInLdap(FALSE);
    $this->attributesAccess['flag_localonly']->setInLdap(FALSE);

    // Email should be unique in all LDAP, not only when used with authentication.
    $this->attributesAccess['mail']->setUnique('whole');

  }

  /* We need $mailAccount->uid to return the uid for mailMethod */
  public function __get ($name)
  {
    if (in_array($name, ['cn', 'uid']) && isset($this->parent)) {
      return $this->parent->getBaseObject()->$name;
    } elseif (in_array($name, ['uidNumber', 'gidNumber']) && isset($this->parent)) {
      return $this->parent->by_object['posixAccount']->$name;
    } else {
      /* Calling default behaviour */
      return parent::__get($name);
    }
  }

  public function __isset ($name)
  {
    if (in_array($name, ['cn', 'uid']) && isset($this->parent)) {
      return isset($this->parent->getBaseObject()->$name);
    } elseif (in_array($name, ['uidNumber', 'gidNumber']) && isset($this->parent)) {
      return isset($this->parent->by_object['posixAccount']->$name);
    } else {
      /* Calling default behaviour */
      return parent::__isset($name);
    }
  }

  function foreignKeyUpdate (string $field, $oldvalue, $newvalue, array $source)
  {
    if ($field == 'gosaMailServer') {
      mailMethod::resetMailServersCache();
      $this->attributesAccess['gosaMailServer']->setChoices(array_keys(mailMethod::getMailServers()));
    }
    parent::foreignKeyUpdate($field, $oldvalue, $newvalue, $source);
  }

  /*!
   * \brief This function is called on the copied object to set its dn to where it will be saved
   */
  function resetCopyInfos ()
  {
    parent::resetCopyInfos();
    $this->initialMailMethod = NULL;
  }

  public function mailServerChanged ()
  {
    $this->mailMethod = mailMethod::getInstanceFromServer($this->gosaMailServer, $this);
    if ($this->initialMailMethod === NULL) {
      $this->initialMailMethod = $this->mailMethod;
    }

    /* Initialize configured values */
    if (!$this->is_template && $this->initially_was_account) {
      if ($this->mailMethod->connect() && $this->mailMethod->account_exists()) {
        /* Read quota */
        $this->attributesAccess['gosaMailQuota']->setMailMethod($this->mailMethod);
        foreach (array_keys($this->attributesInfo['main']['attrs']) as $oldattr) {
          if (!in_array($oldattr, $this->mainSectionAttrs)) {
            $this->removeAttribute('main', $oldattr);
          }
        }
        $addInfos = $this->mailMethod->additionalInformations();
        foreach ($addInfos as $label => $info) {
          $this->addAttribute(
            'main',
            new DisplayAttribute(
              $label, $label,
              $label, FALSE,
              $info
            )
          );
        }
      } elseif (!$this->mailMethod->is_connected()) {
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
                       _('Mail method cannot connect: %s'),
                       $this->mailMethod->get_error()
                     ))
        );
        $error->display();
      } elseif ($this->mailMethod->is_error()) {
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
                       _('Mail method error: %s'),
                       $this->mailMethod->get_error()
                     ))
        );
        $error->display();
      }

      $this->attributesAccess['mail']->setDisabled(!$this->mailMethod->isModifyableMail());
      $this->attributesAccess['gosaMailServer']->setDisabled(!$this->mailMethod->isModifyableServer());
    }

    /* Feature switches */

    if ($this->mailMethod->localOnlyEnabled()) {
      $this->attributesAccess['flag_localonly']->setDisabled(FALSE);
      $this->attributesAccess['flag_localonly']->setVisible(TRUE);
    } else {
      $this->attributesAccess['flag_localonly']->setDisabled(TRUE);
      $this->attributesAccess['flag_localonly']->setVisible(FALSE);
      $this->attributesAccess['flag_localonly']->setValue(FALSE);
    }

    if ($this->mailMethod->forwardOnlyEnabled()) {
      $this->attributesAccess['flag_ownmailbox']->setDisabled(FALSE);
    } else {
      $this->attributesAccess['flag_ownmailbox']->setDisabled(TRUE);
      $this->attributesAccess['flag_ownmailbox']->setValue(FALSE);
    }

    if ($this->mailMethod->vacationEnabled()) {
      $this->attributesAccess['flag_vacation']->setDisabled(FALSE);
      $this->attributesAccess['flag_vacation']->setVisible(TRUE);
    } else {
      $this->attributesAccess['flag_vacation']->setDisabled(TRUE);
      $this->attributesAccess['flag_vacation']->setVisible(FALSE);
      $this->attributesAccess['flag_vacation']->setValue(FALSE);
    }

    if ($this->mailMethod->vacationRangeEnabled()) {
      $this->attributesAccess['gosaVacationStop']->setDisabled(FALSE);
      $this->attributesAccess['gosaVacationStart']->setDisabled(FALSE);
      $this->attributesAccess['flag_vacation']->setManagedAttributes(
        [
          'erase' => [
            FALSE => [
              'gosaVacationStart', 'gosaVacationStop', 'gosaVacationMessage'
            ]
          ]
        ]
      );
    } else {
      $this->attributesAccess['gosaVacationStart']->setDisabled(TRUE);
      $this->attributesAccess['gosaVacationStart']->setVisible(FALSE);
      $this->attributesAccess['gosaVacationStart']->setValue('');
      $this->attributesAccess['gosaVacationStop']->setDisabled(TRUE);
      $this->attributesAccess['gosaVacationStop']->setVisible(FALSE);
      $this->attributesAccess['gosaVacationStop']->setValue('');
      $this->attributesAccess['flag_vacation']->setManagedAttributes(
        [
          'erase' => [
            FALSE => [
              'gosaVacationMessage'
            ]
          ]
        ]
      );
    }
  }

  function check (): array
  {
    $errors = parent::check();

    if (!$this->is_template && is_object($this->mailMethod)) {
      $errors = array_merge($errors, $this->mailMethod->check());
    }

    return $errors;
  }

  protected function shouldSave (): bool
  {
    /* mail method might have something to save (like password change from main tab for instance) */
    return TRUE;
  }

  /*!
  *  \brief Should the mail method create the mailbox? (mail server change or tab activation)
  */
  public function isMailboxCreation (): bool
  {
    return (!$this->initially_was_account || $this->attributesAccess['gosaMailServer']->hasChanged());
  }

  public function ldap_save (): array
  {
    if (!empty($this->attrs)) {
      $errors = parent::ldap_save();
      if (!empty($errors)) {
        return $errors;
      }
    }

    $errors = [];

    /* Only do IMAP actions if we are not a template */
    if (!$this->is_template) {
      $this->mailMethod->connect();
      if (!$this->mailMethod->is_connected()) {
        $errors[] = new SimplePluginError(
          $this,
          htmlescape(sprintf(_('Mail method cannot connect: %s'), $this->mailMethod->get_error()))
        );
      } else {
        if ($this->initially_was_account && $this->attributesAccess['gosaMailServer']->hasChanged()) {
          if (
            ($this->initialMailMethod === NULL) ||
            (get_class($this->initialMailMethod) !== mailMethod::getServerMethod($this->attributesAccess['gosaMailServer']->getInitialValue()))
          ) {
            $errors[] = new SimplePluginError(
              $this,
              htmlescape(
                sprintf(
                  'Initial method wrong or missing: %s !== %s',
                  ($this->initialMailMethod === NULL ? 'NULL' : get_class($this->initialMailMethod)),
                  mailMethod::getServerMethod($this->attributesAccess['gosaMailServer']->getInitialValue())
                )
              )
            );
          } elseif (!$this->initialMailMethod->connect() || !$this->initialMailMethod->deleteMailbox()) {
            $errors[] = new SimplePluginError(
              $this,
              htmlescape(sprintf(_('Cannot remove mailbox: %s'), $this->initialMailMethod->get_error()))
            );
          }
        }
        if (!$this->mailMethod->updateMailbox()) {
          $errors[] = new SimplePluginError(
            $this,
            htmlescape(sprintf(_('Cannot update mailbox: %s'), $this->mailMethod->get_error()))
          );
        }
        if (!$this->mailMethod->setQuota($this->gosaMailQuota)) {
          $error = new SimplePluginError(
            $this,
            htmlescape(sprintf(
                         _('Cannot write quota settings: %s'),
                         $this->mailMethod->get_error()
                       ))
          );
          $error->display();
        }

        if (!is_integer(strpos($this->gosaMailDeliveryMode, 'C'))) {
          /* Do not write sieve settings if this account is new and
             doesn't seem to exist. */
          if (!$this->initially_was_account && !$this->mailMethod->account_exists()) {
            @DEBUG(DEBUG_MAIL, __LINE__, __FUNCTION__, __FILE__,
                   'Skipping sieve settings, the account doesn’t seem to be created already.', '');
          } else {
            if (!$this->mailMethod->saveSieveSettings()) {
              $error = new SimplePluginError(
                $this,
                htmlescape(sprintf(
                             _('Mail error saving sieve settings: %s'),
                             $this->mailMethod->get_error()
                           ))
              );
              $error->display();
            }
          }
        } else {
          if ($this->sieve_management) {
            @DEBUG(DEBUG_MAIL, __LINE__, __FUNCTION__, __FILE__,
                   'User uses an own sieve script, skipping sieve update.', '');
            $this->sieve_management->save();
          }
        }
      }
      $this->mailMethod->disconnect();
    }

    if (!empty($errors) && !$this->initially_was_account) {
      /* Mail method failed to create, cancel ldap save */
      $this->prepareNextCleanup();
      $this->prepare_remove();
      $this->ldap_remove();
    }

    return $errors;
  }

  function post_remove ()
  {
    /* Let the mailMethod remove this mailbox */
    if (!$this->is_template) {
      if (!$this->initialMailMethod->connect()) {
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
                       _('Cannot remove mailbox, mail method cannot connect: %s'),
                       $this->initialMailMethod->get_error()
                     ))
        );
        $error->display();
      } elseif (!$this->initialMailMethod->deleteMailbox()) {
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
                       _('Cannot remove mailbox: %s'),
                       $this->initialMailMethod->get_error()
                     ))
        );
        $error->display();
      }
      $this->initialMailMethod->disconnect();
    }
    parent::post_remove();
  }
}
