
'use strict';


cliento($ => {
    const cdn = 'http://esp8266test.local/api/';

    (_body => {
        (_devices => {
            const outlets = [];
            _devices._('h1', 'Assign Outlets');
            
            (_form => {
                (_fields => {
                    populate(_fields._('label', { for: 'top' }, 'Top Outlet:')._('select', { id: 'top', name: 'top' }));
                    populate(_fields._('label', { for: 'bottom' }, 'Bottom Outlet:')._('select', { id: 'bottom', name: 'bottom' }));

                    $submit(_fields, _form, 'Apply', complete => {
                        const body = new URLSearchParams(new FormData(_form));

                        fetch(cdn + 'devices', {
                            method: 'POST',
                            mode: 'cors',
                            cache: 'no-cache',
                            credentials: 'same-origin',
                            headers: {
                                'Content-Type': 'application/x-www-form-urlencoded',
                            },
                            referrerPolicy: 'no-referrer',
                            body
                        }).then(response => {
                            if (response.ok) {
                                response.text().then(text => {
                                    console.log(text);
                                });
                            } else {
                                console.log('failed');
                            }
                            
                            complete();
                        });
                    });

                    function populate(_outlet) {
                        _outlet._('option', { value: 'none', selected: true, disabled: true, hidden: true }, 'Choose an Outlet');
                        outlets.push(_outlet);
                    }
                })(_form._('fieldset.&col'));
            })(_devices._('form'));

            _body.removeChild(_devices);

            (_login => {
                _login._('h1', 'Login.');

                (_form => {
                    (_fields => {
                        _fields._('label', {for: 'username'})._('input', {
                            id: 'username',
                            type: 'email',
                            name: 'username',
                            placeholder: 'Email Address',
                            autocomplete: 'email',
                            spellcheck: false,
                            required: true
                        });

                        _fields._('label', {for: 'password'})._('input', {
                            id: 'password',
                            type: 'password',
                            name: 'password',
                            placeholder: 'Password',
                            autocomplete: 'current-password',
                            spellcheck: false,
                            required: true
                        });

                        $submit(_fields, _form, 'Submit', complete => {
                            const body = new URLSearchParams(new FormData(_form));

                            fetch(cdn + 'credentials', {
                                method: 'POST',
                                mode: 'cors',
                                cache: 'no-cache',
                                credentials: 'same-origin',
                                headers: {
                                    'Content-Type': 'application/x-www-form-urlencoded',
                                },
                                referrerPolicy: 'no-referrer',
                                body
                            }).then(response => {
                                if (response.ok) {
                                    fetch(cdn + 'devices').then(response => {
                                        if (response.ok) {
                                            response.json().then(devices => {
                                                outlets.forEach(_outlet => {
                                                    devices.forEach(the => {
                                                        _outlet._('option', {value: the.cid}, the.deviceName);
                                                    });
                                                });
                                            });
                                        }
                                    });

                                    response.text().then(text => {
                                        _body.removeChild(_login);
                                        _body.appendChild(_devices);
                                    });
                                } else {
                                    console.log('failed');
                                }

                                complete();
                            });
                        });
                    })(_form._('fieldset.flexcol'));
                })(_login._('form', { name: 'login' }));
            })(_body._('main.login.flexcol'));
        })(_body._('main.devices.flexcol'));
    })($('body'));
});


function $submit(_host, _form, name, submit) {
    ((_submit, _spinner, nodes, index = 0) => {
        while (nodes--) _spinner._('b', { style: { 'animation-delay': index++ * 100 + 'ms' } });
        _host.removeChild(_spinner);

        _form.addEventListener('submit', event => {
            event.preventDefault();

            _host.removeChild(_submit);
            _host.appendChild(_spinner);

            submit(() => {
                _host.removeChild(_spinner);
                _host.appendChild(_submit);
            });
        });
    })(_host._('button', name), _host._('div.spinner'), 3);
}