<?php
/**
 * @file
 * Provides API implementation of MailJet.
 */

use \Mailjet\Resources;

class MailJetSimple {
  private $private_key;
  private $public_key;

  protected $api;

  /**
   * Constructor method to bootstrap the MailJet API.
   *
   * @param $public_key
   * @param $private_key
   *
   * @throws \Exception
   */
  public function __construct($public_key, $private_key) {
    $this->public_key = $public_key;
    $this->private_key = $private_key;
    $this->api = new \Mailjet\Client($public_key, $private_key);
    if (!(is_a($this->api, '\MailJet\Client'))) {
      throw new Exception('Unable to invoke the MailJet Client API');
    }
  }

  /**
   * Makes sure the API endpoint is available.
   *
   * @return bool
   * @throws \Exception
   */
  public function ping() {
    // Ping the api key access url with a 5 second timeout.
    $ping_url = 'https://api.mailjet.com/v3/REST/apikeyaccess';
    $guzzle = new \GuzzleHttp\Client(['timeout' => 5]);

    // Add authentication parameters.
    $options = [
      'auth' => [
        $this->public_key,
        $this->private_key
      ]
    ];

    // Attempt to ping and authenticate with the endpoint.
    // Throw exceptions whenever a ping is unsuccessful.
    try {
      $ping = $guzzle->request('GET', $ping_url, $options);
    }
    catch (Exception $ex) {
      if ($ex->getCode() == 401) {
        throw new Exception('Unable to authenticate with MailJet. Please check your API keys.');
      }
      throw $ex;
    }

    if (empty($ping)) {
      throw new \Exception('Unable to ping the MailJet API');
    }

    return TRUE;
  }

  /**
   * Fetches the list information for a contact by their email.
   *
   * @param $email string
   *
   * @return array
   *   An array of lists if any were loaded.
   */
  public function getListsByEmail($email) {
    $contact = $this->contactByEmail($email);
    if (!empty($contact)) {
      return $this->getListsByContact($contact['ID']);
    }
    return [];
  }

  /**
   * Fetches the list subscription records for a contact.
   *
   * @param $id integer.
   *   The MailJet Id of the contact object.
   *
   * @return array
   *   An array of list data
   *   http://dev.mailjet.com/email-api/v3/contact-getcontactslists.
   */
  public function getListsByContact($id) {
    $response = $this->api->get(Resources::$ContactGetcontactslists, ['id' => $id]);
    if ($response->success()) {
      return $response->getData();
    }
    return [];
  }

  /**
   * Fetches a MailJet Contact record by email address.
   * @param $email
   * @return mixed|null
   */
  public function contactByEmail($email) {
    // Load the recipient record by email.
    $filters = [
      'ContactEmail' => $email
    ];
    $response = $this->api->get(Resources::$Listrecipient, ['filters' => $filters]);

    // Fetch the contact record from the recipient response.
    if ($response->success()) {
      // One record per subscription is returned, we only need the first item to get the ID.
      $recipients = $response->getData();
      $recipient = reset($recipients);
      if (!empty($recipient)) {
        $contact_response = $this->api->get(Resources::$Contact, ['id' => $recipient['ContactID']]);
        if ($contact_response->success()) {
          // Only one contact is returned, but it's an array of results.
          $contacts = $contact_response->getData();
          return reset($contacts);
        }
      }
    }

    return NULL;
  }


  /**
   * Updates a contacts subscription to a particular list.
   *
   * @param $id integer
   *   The MailJet contact object id.
   * @param $list integer
   *   The MailJet list object id.
   * @param $action string
   *   The action to take on the list.
   *   Options:
   *     - addforce
   *     - addnoforce
   *     - remove
   *     - unsub
   *
   * @return bool
   */
  public function updateListsByContact($id, $list, $action) {
    $body = [
      'ContactsLists' => [
        [
          'ListID' => $list,
          'Action' => $action
        ]
      ]
    ];
    $response = $this->api->post(Resources::$ContactManagecontactslists, ['id' => $id, 'body' => $body]);
    return $response->success();
  }

  /**
   * Creates a new MailJet contact and subscribes to lists.
   *
   * @param $values
   *   The values for the contact.
   *   Options:
   *     - Email
   *     - Name
   *     - Properties
   *         - Property 1
   *         - Property 2
   * @param array $lists
   *   An optional array of list ids and actions.
   *   Options:
   *     - ListId
   *     - Action (addforce, addnoforce, remove, unsub).
   *
   * @return boolean
   *
   */
  public function updateContact($values, $lists = []) {
    $body = [
      'Contacts' => [
        $values
      ]
    ];

    if (!empty($lists)) {
      $body['ContactsLists'] = $lists;
    }

    $response = $this->api->post(Resources::$ContactManagemanycontacts, ['body' => $body]);

    return $response->success();
  }

  /**
   * Loads the available Mailjet lists.
   *
   * @return array
   */
  public function getContactLists() {
    $response = $this->api->get(Resources::$Contactslist);
    if ($response->success()) {
      return (array) $response->getData();
    }
    return [];
  }
}
