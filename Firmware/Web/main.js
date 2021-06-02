
function populateOutlets() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", 'http://esp8266test.local/api/devices', true);
    xhr.onreadystatechange = function () { // Call a function when the state changes.
        if (this.readyState === XMLHttpRequest.DONE) {
            if (this.status === 200) {
                //success
                console.log(xhr.responseText);
                var outletForm = document.forms.namedItem("devices");
                var outletData = JSON.parse(xhr.responseText);
                outletData.forEach(thisOutlet => {
                    console.log(thisOutlet.deviceName);
                    outletForm.querySelectorAll('select').forEach(thisSelect => {
                        var outletOption = document.createElement('option');
                        outletOption.innerText = thisOutlet.deviceName;
                        outletOption.setAttribute('value', thisOutlet.cid);
                        thisSelect.appendChild(outletOption);
                    });
                });

                nextSlide();
            } else {
                //failure
                console.log('could not populate outlets');
            }
        }
    }
    xhr.send();
}

function nextSlide() {
    var slides = document.querySelectorAll('.centerizer');
    var currentSlide = document.querySelector('.centerizer.active');
    for (let index = 0; index < slides.length; index++) {
        if (slides.item(index) == currentSlide) {
            var nextSlide = slides.item((index + 1) % slides.length);
            currentSlide.classList.remove('active');
            setTimeout(() => {
                nextSlide.classList.add('active');
            }, 500);
        }
    }
}

function submitForm(endpoint, ev, successCallback) {
    ev.preventDefault();
    var serialFormData = new URLSearchParams(new FormData(ev.target)).toString();

    var fieldset = ev.target.querySelector('fieldset');
    fieldset.disabled = true;


    var xhr = new XMLHttpRequest();
    xhr.open("POST", 'http://esp8266test.local/api/' + endpoint, true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function () { // Call a function when the state changes.
        if (this.readyState === XMLHttpRequest.DONE) {
            if (this.status === 200) {
                //success
                console.log(xhr.responseText);
                successCallback();
            } else {
                //failure, present login options again
                fieldset.disabled = false;
            }
        }
    }
    xhr.send(serialFormData);
}

document.addEventListener('DOMContentLoaded', function () {
    fetch("https://cdn.jsdelivr.net/gh/christian-kramer/Workbench-Light-Controller/Firmware/Web/main.html").then(res => {
        res.text().then((text) => {
            document.querySelector('HTML').innerHTML = text;
            
            var loginForm = document.forms.namedItem("login");
            loginForm.addEventListener('submit', function (ev) {
                submitForm('credentials', ev, function () {
                    populateOutlets();
                })
            });

            var devicesForm = document.forms.namedItem("devices");
            devicesForm.addEventListener('submit', function (ev) {
                submitForm('devices', ev, function () {
                    setTimeout(() => {
                        devicesForm.querySelector('fieldset').disabled = false;
                    }, 1000);
                })
            });
        });
    });
    
}, false);